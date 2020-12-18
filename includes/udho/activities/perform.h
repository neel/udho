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

#ifndef UDHO_ACTIVITIES_PERFORM_H
#define UDHO_ACTIVITIES_PERFORM_H

#include <cstdint>
#include <ostream>
#include <udho/activities/subtask.h>

namespace udho{
/**
 * \ingroup activities
 */
namespace activities{
    
    /**
     * create a subtask to perform activity ActivityT
     * \tparam ActivityT the activity to perform
     * \ingroup activities
     */
    template <typename ActivityT>
    struct perform{
        /**
         * mention the activities that has to be performed before executing this subtask.
         * \tparam DependenciesT dependencies
         */
        template <typename... DependenciesT>
        struct require{
            /**
             * arguments for the activity constructor
             */
            template <typename... U>
            static subtask<ActivityT, DependenciesT...> with(U&&... u){
                return subtask<ActivityT, DependenciesT...>::with(u...);
            }
        };
        
       /**
        * arguments for the activity constructor
        */
        template <typename... U>
        static subtask<ActivityT> with(U&&... u){
            return subtask<ActivityT>::with(u...);
        }
    };
    
}

}

#endif // UDHO_ACTIVITIES_PERFORM_H

