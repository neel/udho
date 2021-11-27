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
#include <udho/activities/fwd.h>
#include <udho/activities/collector.h>
#include <udho/activities/accessor.h>
#include <udho/activities/after.h>

namespace udho{
namespace activities{
    /**
     * @brief An empty initial activity that starts the chain of activities
     * Constructs a collector to collect the result of the activities provided in Activities... using the provided context.
     * This is a special activity that always succeeds and cannot be canceled. The activity is invoked via its operator() 
     * which succeeds and invokes all other activities that depend on this activity,
     * @tparam ContextT 
     * @tparam Activities... List of activities
     * @ingroup activities
     */
    template <typename ContextT, typename... Activities>
    struct init{
        typedef activities::collector<ContextT, Activities...> collector_type;
        typedef activities::accessor<Activities...> accessor_type;
        typedef std::shared_ptr<collector_type> collector_ptr;
        typedef boost::signals2::signal<void ()> signal_type;
        
        init(ContextT& ctx): _collector(std::make_shared<collector_type>(ctx)), _accessor(_collector){}
        
        /**
         * @brief get the collector used for the activity graph
         * 
         * @return collector_ptr 
         */
        collector_ptr collector() const { return _collector; }
        /**
         * @brief get the accessor associated with the collector
         * 
         * @return const accessor_type& 
         */
        const accessor_type& accessor() const { return _accessor; }
        /**
         * @brief get the accessor associated with the collector
         * 
         * @return accessor_type& 
         */
        accessor_type& accessor() { return _accessor; }
        
        /**
         * @brief invoke the activity, which always succeeds and as a result it starts the activity graph.
         * 
         */
        void operator()(){ success(); }
        
        template <typename CombinatorT>
        void done(CombinatorT cmb){
            boost::function<void ()> fnc([cmb](){
                cmb->operator()();
            });
            _signal.connect(fnc);
        }

        private:
            void success(){ _signal(); }

            signal_type   _signal;
            collector_ptr _collector;
            accessor_type _accessor;
    };

    template <typename ContextT, typename... Activities>
    struct collector_of<init<ContextT, Activities...>>{
        using type = collector<ContextT, Activities...>;
        static std::shared_ptr<type> apply(std::shared_ptr<init<ContextT, Activities...>> init){ return init->collector(); }
    };
    
#ifndef __DOXYGEN__

    /**
     * @brief combinator specialized for init activity
     * This combinator cannot be prepared, because no other subtask is completed before it.
     * @tparam NextT 
     * @tparam ContextT 
     * @tparam T 
     */
    template <typename NextT, typename ContextT, typename... T>
    struct combinator<NextT, init<ContextT, T...>>{
        typedef std::shared_ptr<NextT> next_type;
        next_type  _next;

        combinator(next_type& next): _next(next){}
        void operator()(){
            propagate();
        }
        void propagate(){
            (*_next)();
        }
        
        /**
         * A subtask with no dependency cannot be prepared, because no other subtask has completed before it.
         */
        template <typename PreparatorT>
        void prepare(PreparatorT){
            static_assert(true, "A subtask with no dependency cannot be prepared, because no other subtask has completed before it.");
        }
    };
    
    /**
     * @brief subtask specialized for init activity
     * This subtask does not depend on any other subtasks. So it does not have a combinator
     * @tparam ContextT 
     * @tparam T 
     */
    template <typename ContextT, typename... T>
    struct subtask<init<ContextT, T...>>{
        typedef init<ContextT, T...> activity_type;
        typedef subtask<init<ContextT, T...>> self_type;
        
        template <typename U, typename... DependenciesU>
        friend struct subtask;
        
        subtask(const self_type& other): _activity(other._activity){}
                
        inline std::shared_ptr<activity_type> activity_ptr() { return _activity; }
        inline activity_type& activity(){ return *(_activity.get()); }
        inline activity_type& operator*(){ return activity(); }
        inline std::shared_ptr<activity_type> operator->(){ return _activity; }
        
        /**
         * @brief execute task next after the current one
         * @param next the next subtask
         */
        template <typename V, typename... DependenciesV>
        self_type& done(subtask<V, DependenciesV...>& next){
            activity()->done(next._combinator);
            return *this;
        }
        
        /**
         * @brief Arguments for the constructor of the Activity
         */
        static self_type with(ContextT ctx){
            return self_type(ctx);
        }
        
        /**
         * @brief calls the `operator()()` of the activity and starts executing the graph
         */
        void operator()(){
            _activity->operator()();
        }
        
        protected:
            subtask(ContextT ctx){
                _activity = std::shared_ptr<activity_type>(new activity_type(ctx));
            }
            
            std::shared_ptr<activity_type> _activity;
    };
#endif 

/**
 * @brief A wrapper around the subtask for init activity.
 * @see start
 * @tparam ContextT 
 * @tparam T 
 * @ingroup activities
 */
template <typename ContextT, typename... T>
struct starter: activities::subtask<activities::init<ContextT, T...>>{
    typedef activities::init<ContextT, T...> activity_type;
    typedef activities::subtask<activity_type> base;
    typedef typename activity_type::collector_type collector_type;
    typedef typename activity_type::collector_ptr collector_ptr;
    typedef typename activity_type::accessor_type accessor_type;
    
    /**
     * @brief Construct a new starter object using the context
     * 
     * @param ctx 
     */
    explicit starter(ContextT ctx): base(ctx){}
    
    /**
     * @brief get the collector for the activity graph
     * 
     * @return collector_ptr 
     */
    inline collector_ptr collector() { return base::_activity->collector(); }
    /**
     * @brief get the collector for the activity graph
     * 
     * @return collector_ptr 
     */
    inline collector_ptr data() const { return collector(); }
    /**
     * @brief get the collector for the activity graph
     * 
     * @return collector_ptr 
     */
    inline collector_ptr data() { return collector(); }
    /**
     * @brief get the accessor for the activity graph
     * 
     * @return const accessor_type& 
     */
    inline const accessor_type& accessor() const { return base::_activity->accessor(); }
    /**
     * @brief get the accessor for the activity graph
     * 
     * @return accessor_type& 
     */
    inline accessor_type& accessor() { return base::_activity->accessor(); }
    
    /**
     * @brief get success data of some activity in the activity graph
     * 
     * @tparam ActivityT 
     * @return auto 
     */
    template <typename ActivityT>
    auto success() const { return accessor().template success<ActivityT>(); }
    /**
     * @brief get failure data of some activity in the activity graph
     * 
     * @tparam ActivityT 
     * @return auto 
     */
    template <typename ActivityT>
    auto failure() const { return accessor().template failure<ActivityT>(); }
};

/**
 * @brief conveniance function to create a starter for an activity graph
 * 
 * @tparam Activities...
 * @ingroup activities
 */
template <typename... Activities>
struct start{
    /**
     * @brief construct a starter using the context provided.
     * 
     * @tparam ContextT 
     * @param ctx 
     * @return starter<ContextT, Activities...> 
     */
    template <typename ContextT>
    static starter<ContextT, Activities...> with(ContextT ctx){
        return starter<ContextT, Activities...>(ctx);
    }
    
    start() = delete;
    start(const start<Activities...>&) = delete;
};

#ifndef __DOXYGEN__

namespace detail{
    
    template <typename ContextT, typename... T>
    struct after<udho::activities::starter<ContextT, T...>>{
        udho::activities::starter<ContextT, T...>& _before;
        
        after(udho::activities::starter<ContextT, T...>& before): _before(before){}
        
        template <typename OtherActivityT, typename... OtherDependenciesT>
        void attach(subtask<OtherActivityT, OtherDependenciesT...>& sub){
            sub.after(_before);
        }
    };
    
}

#endif 

}
}

#endif // UDHO_ACTIVITIES_START_H

