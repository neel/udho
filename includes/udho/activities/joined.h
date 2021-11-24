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
#include <udho/activities/fwd.h>
#include <udho/activities/collector.h>
#include <udho/activities/accessor.h>
#include <udho/activities/subtask.h>

namespace udho{
/**
 * @ingroup activities
 */
namespace activities{
    
    /**
     * @brief A joined activity is the final activity in the chain that consists of a callback which is called with an accessor compatiable with the collector.
     * @note the callback is called irrespective of whether its dependencies have succeed, failed or canceled.
     * There can be multiple callbacks joined in the leaf of activity tree.
     * @warning a joined subtask cannot be used as a dependency of another subtask.
     * @see basic_after::finish
     * @ingroup activities
     */
    template <typename CallbackT, typename... T, typename ContextT>
    struct joined<CallbackT, activities::collector<ContextT, T...>>{
        typedef activities::collector<ContextT, T...> collector_type;
        typedef typename accessor_of<collector_type>::type accessor_type;
        typedef CallbackT callback_type;
        
        joined(std::shared_ptr<collector_type> collector, CallbackT callback): _collector(collector), _callback(callback) {}
        void operator()(){  
            accessor_type accessor(_collector);
            _callback(accessor);
        }
        void cancel(){
            operator()();
        }
        private:
            std::shared_ptr<collector_type> _collector;
            callback_type _callback;
    };

#ifndef __DOXYGEN__

    template <typename CallbackT, typename... X, typename CtxT, typename... DependenciesT>
    struct subtask<joined<CallbackT, activities::collector<CtxT, X...>>, DependenciesT...>{
        typedef subtask<joined<CallbackT, activities::collector<CtxT, X...>>, DependenciesT...> self_type;
        typedef joined<CallbackT, activities::collector<CtxT, X...>> activity_type;
        typedef combinator<activity_type, DependenciesT...> combinator_type;

        template <typename U, typename... DependenciesU>
        friend struct subtask;

        subtask(const self_type& other): _activity(other._activity), _combinator(other._combinator), _interaction(other._interaction){}

        std::shared_ptr<activity_type> activity_ptr() { return _activity; }
        activity_type& activity(){ return *(_activity.get()); }
        activity_type& operator*(){ return activity(); }
        std::shared_ptr<activity_type> operator->(){ return _activity; }

        /**
         * execute task next after the current one
         * \param next the next subtask
         */
        template <typename V, typename... DependenciesV>
        self_type& done(subtask<V, DependenciesV...>& next){
            activity_ptr()->done(next._combinator);
            return *this;
        }
        
        /**
         * t2.after(t1) is equivalent to t1.done(t2)
         * \param previous the previous subtask
         */
        template <typename V, typename... DependenciesV>
        self_type& after(subtask<V, DependenciesV...>& previous){
            previous._activity->done(_combinator);
            return *this;
        }
        
        /**
         * Arguments for the constructor of the Activity
         */
        template <typename ContextT, typename... T, typename... U>
        static self_type with(std::shared_ptr<udho::activities::collector<ContextT, T...>> collector_ptr, U&&... u){
            return self_type(collector_ptr, u...);
        }
        
        /**
         * attach a callback which will be called with a reference to the activity after it has been instantiated and all its dependencies have completed.
         */
        template <typename PreparatorT>
        self_type& prepare(PreparatorT prep){
            _combinator->prepare(prep);
            return *this;
        }

        /**
         * Set required flag on or off. If a required subtask fails then all intermediate subtask that depend on it fails and the final callback is called immediately. By default all subtasks are required
         */
        self_type& required(bool flag){ _activity->required(flag); return *this; }

        protected:
            template <typename ContextT, typename... T, typename... U>
            subtask(std::shared_ptr<udho::activities::collector<ContextT, T...>> collector_ptr, U&&... u): _interaction(collector_ptr->context().interaction()){
                _activity = std::make_shared<activity_type>(collector_ptr, u...);
                _combinator = std::make_shared<combinator_type>(_activity);
            }
            
            std::shared_ptr<activity_type> _activity;
            std::shared_ptr<combinator_type> _combinator;
            udho::detail::interaction_& _interaction;
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
    
#endif

}

}

#endif // UDHO_ACTIVITIES_JOINED_H

