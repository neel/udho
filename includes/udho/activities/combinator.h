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
#include <udho/hazo/node/meta.h>

namespace udho{
namespace activities{
    /**
     * @brief A combinator combines multiple activities and proceeds towards the next activity.
     * @tparam NextT next activity
     * @tparam DependenciesT dependencies
     *
     * Given two activities A1 and A2, in order to express the dependency of A2 on A1 a `combinator<A2, A1>` 
     * is used by the @ref udho::activities::subtask "subtask" internally. a `combinator<X, A, B, C, ...>`
     * combines all dependencies `A, B, C, ...`  and once all the dependencies complete it starts the next 
     * activity `X`, i.e. calls the next activity `X`'s `operator()()`.
     * @attention It is not recomended to construct a combinator directly. Instead use subtasks and other 
     *            convenient functions that constructs and manages the combinator appropriate for the activity 
     *            and the dependencies.
     *
     * Following example demonstrates usage of the combinator.
     * @code 
     * auto collector = activities::collect<A1, A2>(ctx);
     * auto a1 = std::make_shared<A1>(collector);
     * auto a2 = std::make_shared<A2>(collector);
     *  
     * auto combinator = std::make_shared<activities::combinator<A2, A1>>(a2);
     * a1->done(combinator);
     * @endcode 
     * A combinator also provides a `prepare` function which takes a callback as an input which is called 
     * after all its depedencies have completed and before the next activity is invoked.
     * @ingroup activities
     */
    template <typename NextT, typename... DependenciesT>
    struct combinator{
        typedef std::shared_ptr<NextT> next_type;
        typedef boost::signals2::signal<void (NextT&)> signal_type;
        typedef typename udho::hazo::meta<typename DependenciesT::result_type...>::types allowed_inputs;
        
        /**
         * @brief Construct a new combinator object
         * 
         * @param next 
         */
        combinator(next_type& next): _next(next), _counter(sizeof...(DependenciesT)), _canceled(false){}
        
        /**
         * @brief Signal finishing of one subtask connected to this combinator.
         * whenever a subtask finishes the `operator()` of the combinator is called. which doesn't start the next subtask untill all the dependencies have completed.
         * Before starting the next activity the next activity is prepared if any preparator is passed through the `prepare()` function
         * @tparam U Result type of one of the subtasks connected to this combinator.
         * @param u 
         */
        template <typename U, std::enable_if_t<allowed_inputs::template exists<U>::value, bool> = true>
        void operator()(const U& u){
            propagate();
        }
        
        /**
         * @brief Cancel invocation of all child activities
         */
        void cancel(){
            _canceled = true;
            propagate();
        }
        
        /**
         * @brief set a preparator callback which will be called with a reference to teh next activity. 
         * The preparator callback is supposed to prepare the next activity by using the data collected till that time.
         * @tparam PreparatorT 
         * @param prep 
         */
        template <typename PreparatorT>
        void prepare(PreparatorT prep){
            boost::function<void (NextT&)> fnc(prep);
            _preparators.connect(prep);
        }

        private:
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

        private:
            next_type                _next;
            std::atomic<std::size_t> _counter;
            std::mutex               _mutex;
            signal_type              _preparators;
            std::atomic<bool>        _canceled;
    };
    

    
}

}

#endif // UDHO_ACTIVITIES_COMBINATOR_H

