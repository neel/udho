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

#ifndef UDHO_ACTIVITIES_SUBTASK_H
#define UDHO_ACTIVITIES_SUBTASK_H

#include <memory>
#include <udho/activities/combinator.h>
#include <udho/context.h>
#include <udho/activities/fwd.h>

namespace udho{
/**
 * \ingroup activities
 */
namespace activities{
    
    /**
     * A `subtask` is an instantiation of an `activity`. The subtask reuses an activity to model different use cases by attaching dependencies.
     * A subtask contains two shared pointers, one to the activity and another one to the combinator.
     * The subtask cannot be instantiated directly by calling the subtask constructor. Instead call the static `with` method to instantiate.
     * 
     * \tparam ActivityT The  activity 
     * \tparam DependenciesT The activities that has to be performed before performing ActivityT
     * \ingroup activities
     */
    template <typename ActivityT, typename... DependenciesT>
    struct subtask{
        typedef ActivityT activity_type;
        typedef combinator<ActivityT, DependenciesT...> combinator_type;
        typedef subtask<ActivityT, DependenciesT...> self_type;
        
        template <typename U, typename... DependenciesU>
        friend struct subtask;
        
        subtask(const self_type& other): _activity(other._activity), _combinator(other._combinator), _interaction(other._interaction){}
        
        /**
         * shared pointer to the activity
         */
        std::shared_ptr<activity_type> activity() {
            return _activity;
        }
        
        /**
         * execute task next after the current one
         * \param next the next subtask
         */
        template <typename V, typename... DependenciesV>
        self_type& done(subtask<V, DependenciesV...>& next){
            activity()->done(next._combinator);
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
        template <typename CollectorT, typename... U>
        static self_type with(CollectorT collector, U&&... u){
            return self_type(0, collector, u...);
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
        self_type& required(bool flag){
            _activity->required(flag);
            return *this;
        }
        
        /**
         * Force cancelation of the activity even after it is successful to stop propagating to the next activities
         * \param f callback which should return true to signal cancelation
         */
        self_type& cancel_if(typename activity_type::cancel_if_ftor cancelor){
            _activity->cancel_if(cancelor);
            return *this;
        }
        
        /**
         * returns the shared pointer to the actiivity
         */
        std::shared_ptr<activity_type> operator->(){
            return _activity;
        }
        
        /**
         * abort if canceled if ftor returns false. f will be called with the success if it has been canceled due to error
         */
        template <typename FunctionT>        
        self_type& if_errored(FunctionT ftor){
            _activity->if_errored(ftor);
            return *this;
        }
        
        /**
         * abort if canceled if ftor returns false. f will be called with the failue data if it has been canceled due to failure
         */
        template <typename FunctionT>
        self_type& if_failed(FunctionT ftor){
            _activity->if_failed(ftor);
            return *this;
        }
        
        template <typename FunctionT>
        self_type& if_canceled(FunctionT ftor){
            typename activity_type::abort_error_ftor ef = [ftor](const typename activity_type::success_type& s) mutable{
                return ftor(s);
            };
            
            typename activity_type::abort_failure_ftor ff = [ftor](const typename activity_type::failure_type& f) mutable{
                return ftor(f);
            };
            if_errored(ef);
            if_failed(ff);
            return *this;
        }
        
        private:
            template <typename CollectorT, typename... U>
            subtask(int, CollectorT collector, U&&... u): _interaction(collector->context().interaction()){
                _activity = std::make_shared<activity_type>(collector, u...);
                _combinator = std::make_shared<combinator_type>(_activity);
            }
            
            std::shared_ptr<activity_type> _activity;
            std::shared_ptr<combinator_type> _combinator;
            udho::detail::interaction_& _interaction;
    };
    
    /**
     * Spetialization for the root subtask in the task graph
     * 
     * \tparam ActivityT The  activity 
     * \ingroup activities
     */
    template <typename ActivityT>
    struct subtask<ActivityT>{
        typedef ActivityT activity_type;
        typedef subtask<ActivityT> self_type;
        
        template <typename U, typename... DependenciesU>
        friend struct subtask;
        
        subtask(const self_type& other): _activity(other._activity), _interaction(other._interaction){}
                
        std::shared_ptr<activity_type> activity() {
            return _activity;
        }
        
        /**
         * execute task next after the current one
         * \param next the next subtask
         */
        template <typename V, typename... DependenciesV>
        self_type& done(subtask<V, DependenciesV...>& next){
            activity()->done(next._combinator);
            return *this;
        }
        
        /**
         * Arguments for the constructor of the Activity
         */
        template <typename CollectorT, typename... U>
        static self_type with(CollectorT collector, U&&... u){
            return self_type(0, collector, u...);
        }
        
        /**
         * calls the `operator()()` of the activity and starts executing the graph
         */
        template <typename... U>
        void operator()(U&&... u){
            _activity->operator()(u...);
        }
        
        /**
         * Set required flag on or off. If a required subtask fails then all intermediate subtask that depend on it fails and the final callback is called immediately. By default all subtasks are required
         */
        self_type& required(bool flag){
            _activity->required(flag);
            return *this;
        }
        
        /**
         * Force cancelation of the activity even after it is successful to stop propagating to the next activities
         * \param f callback which should return true to signal cancelation
         */
        self_type& cancel_if(typename activity_type::cancel_if_ftor cancelor){
            _activity->cancel_if(cancelor);
            return *this;
        }
        
        /**
         * returns the shared pointer to the actiivity
         */
        std::shared_ptr<activity_type> operator->(){
            return _activity;
        }
        
        /**
         * abort if canceled if ftor returns false. f will be called with the success if it has been canceled due to error
         */
        template <typename FunctionT>        
        self_type& if_errored(FunctionT ftor){
            _activity->if_errored(ftor);
            return *this;
        }
        
        /**
         * abort if canceled if ftor returns false. f will be called with the failue data if it has been canceled due to failure
         */
        template <typename FunctionT>
        self_type& if_failed(FunctionT ftor){
            _activity->if_failed(ftor);
            return *this;
        }
        
        template <typename FunctionT>
        self_type& if_canceled(FunctionT ftor){
            typename activity_type::abort_error_ftor   ef = ftor;
            typename activity_type::abort_failure_ftor ff = ftor;
            if_errored(ef);
            if_failed(ff);
            return *this;
        }
        
        private:
            template <typename CollectorT, typename... U>
            subtask(int, CollectorT collector, U&&... u): _interaction(collector->context().interaction()){
                _activity = std::make_shared<activity_type>(collector, u...);
            }
            
            std::shared_ptr<activity_type> _activity;
            udho::detail::interaction_& _interaction;
    };
    
}

}

#endif // UDHO_ACTIVITIES_SUBTASK_H
