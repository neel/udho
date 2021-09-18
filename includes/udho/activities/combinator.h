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

#ifndef UDHO_ACTIVITIES_COMBINATOR_H
#define UDHO_ACTIVITIES_COMBINATOR_H

#include <cstdint>
#include <ostream>
#include <memory>
#include <atomic>
#include <mutex>
#include <boost/signals2.hpp>
#include <udho/activities/fwd.h>

namespace udho{
/**
 * \ingroup activities
 */
namespace activities{
    
    /**
     * \internal 
     */
    template <typename DependencyT>
    struct junction{
        void operator()(const DependencyT&){}
    };
    
    /**
     * A combinator combines multiple activities and proceeds towards the next activity
     * \tparam NextT next activity
     * \tparam DependenciesT dependencies
     * \ingroup activities
     */
    template <typename NextT, typename... DependenciesT>
    struct combinator: junction<typename DependenciesT::result_type>...{
        typedef std::shared_ptr<NextT> next_type;
        typedef boost::signals2::signal<void (NextT&)> signal_type;
        
        next_type  _next;
        std::atomic<std::size_t> _counter;
        
        std::mutex  _mutex;
        signal_type _preparators;
        std::atomic<bool> _canceled;
        
        combinator(next_type& next): _next(next), _counter(sizeof...(DependenciesT)), _canceled(false){}
        
        /**
         * whenever a subtask finishes the `operator()` of the combinator is called. which doesn't start the next subtask untill all the dependencies have completed.
         * Before starting the next activity the next activity is prepared if any preparator is passed through the `prepare()` function
         */
        template <typename U>
        void operator()(const U& u){
            junction<U>::operator()(u);
            propagate();
        }
        
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
        
        /**
         *set a preparator callback which will be called with a reference to teh next activity. The preparator callback is supposed to prepare the next activity by using the data callected till that time.
         */
        template <typename PreparatorT>
        void prepare(PreparatorT prep){
            boost::function<void (NextT&)> fnc(prep);
            _preparators.connect(prep);
        }
    };
    

    
}

}

#endif // UDHO_ACTIVITIES_COMBINATOR_H

