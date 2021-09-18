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

#ifndef UDHO_ACTIVITIES_START_H
#define UDHO_ACTIVITIES_START_H

#include <atomic>
#include <mutex>
#include <memory>
#include <string>
#include <boost/signals2.hpp>
#include <boost/function.hpp>
#include <udho/activities/data.h>
#include <udho/activities/fwd.h>

namespace udho{
/**
 * \ingroup activities
 */
namespace activities{
    
    template <typename ContextT, typename... T>
    struct start{
        typedef activities::collector<ContextT, activities::dataset<T...>> collector_type;
        typedef activities::accessor<T...> accessor_type;
        typedef std::shared_ptr<collector_type> collector_ptr;
        typedef boost::signals2::signal<void ()> signal_type;
        
        signal_type   _signal;
        collector_ptr _collector;
        accessor_type _accessor;
        
        start(ContextT& ctx, const std::string& name): _collector(std::make_shared<collector_type>(ctx, name)), _accessor(_collector){}
        
        collector_ptr collector() const { return _collector; }
        const accessor_type& accessor() const { return _accessor; }
        accessor_type& accessor() { return _accessor; }
        
        void operator()(){
            success();
        }
        
        template <typename CombinatorT>
        void done(CombinatorT cmb){
            boost::function<void ()> fnc([cmb](){
                cmb->operator()();
            });
            _signal.connect(fnc);
        }
        
        void success(){                
            _signal();
        }
    };
    
    template <typename NextT, typename ContextT, typename... T>
    struct combinator<NextT, start<ContextT, T...>>{
        typedef std::shared_ptr<NextT> next_type;
        typedef boost::signals2::signal<void (NextT&)> signal_type;

        next_type  _next;

        std::atomic<std::size_t> _counter;

        std::mutex  _mutex;
        signal_type _preparators;
        std::atomic<bool> _canceled;

        combinator(next_type& next): _next(next), _counter(1) {}

        void cancel(){
            _canceled = true;
            propagate();
        }
        
        void propagate(){
            _counter--;
            if(!_counter){
                _mutex.lock();
                if(_canceled){
                    _next->cancel();
                }else{
                    if(!_preparators.empty()){
                        _preparators(*_next);
                        _preparators.disconnect_all_slots();
                    }
                    (*_next)();
                }
                _mutex.unlock();
            }
        }
        
        void operator()(){
            propagate();
        }
        
        /**
         *set a preparator callback which will be called with a reference to teh next activity. The preparator callback is supposed to prepare the next activity by using the data callected till that time.
         */
        template <typename PreparatorT>
        void prepare(PreparatorT prep){
            boost::function<void (NextT&)> fnc(prep);
            _preparators.connect(prep);
        }
    };
    
    template <typename ContextT, typename... T>
    struct subtask<start<ContextT, T...>>{
        typedef start<ContextT, T...> activity_type;
        typedef subtask<start<ContextT, T...>> self_type;
        
        template <typename U, typename... DependenciesU>
        friend struct subtask;
        
        subtask(const self_type& other): _activity(other._activity){}
                
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
        static self_type with(ContextT ctx, const std::string& name = ""){
            return self_type(ctx, name);
        }
        
        /**
         * calls the `operator()()` of the activity and starts executing the graph
         */
        void operator()(){
            _activity->operator()();
        }
        
        /**
         * returns the shared pointer to the actiivity
         */
        std::shared_ptr<activity_type> operator->(){
            return _activity;
        }
        
        protected:
            subtask(ContextT ctx, const std::string& name = ""){
                _activity = std::shared_ptr<activity_type>(new activity_type(ctx, name));
            }
            
            std::shared_ptr<activity_type> _activity;
    };
    
}

template <typename ContextT, typename... T>
struct start_: activities::subtask<activities::start<ContextT, T...>>{
    typedef activities::start<ContextT, T...> activity_type;
    typedef activities::subtask<activity_type> base;
    typedef typename activity_type::collector_type collector_type;
    typedef typename activity_type::accessor_type accessor_type;
    
    start_(ContextT ctx, const std::string& name = "activity"): base(ctx, name){}
    
    auto collector() { return base::_activity->collector(); }
    auto data() const { return base::_activity->collector(); }
    auto data() { return base::_activity->collector(); }
    
    template <typename ActivityT>
    auto success() const {
        return udho::activities::accessor<ActivityT>(data()).template success<ActivityT>();
    }
    template <typename ActivityT>
    auto failure() const {
        return udho::activities::accessor<ActivityT>(data()).template failure<ActivityT>();
    }
};

template <typename... T>
struct start{
    template <typename ContextT>
    static start_<ContextT, T...> with(ContextT ctx, const std::string& name = "activity"){
        return start_<ContextT, T...>(ctx, name);
    }
    
    start() = delete;
    start(const start<T...>&) = delete;
};

namespace activities{
    namespace detail{
        
        template <typename ContextT, typename... T>
        struct after<udho::start_<ContextT, T...>>{
            udho::start_<ContextT, T...>& _before;
            
            after(udho::start_<ContextT, T...>& before): _before(before){}
            
            template <typename OtherActivityT, typename... OtherDependenciesT>
            void attach(subtask<OtherActivityT, OtherDependenciesT...>& sub){
                sub.after(_before);
            }
        };
        
    }
}

}

#endif // UDHO_ACTIVITIES_START_H

