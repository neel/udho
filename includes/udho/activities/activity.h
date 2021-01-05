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

#ifndef UDHO_ACTIVITIES_ACTIVITY_H
#define UDHO_ACTIVITIES_ACTIVITY_H

#include <memory>
#include <udho/activities/result.h>

namespace udho{
/**
 * \ingroup activities
 */
namespace activities{
    
    /**
     * An activity `A` must subclass from `activity<A, SuccessA, FailureA>` assuming `SuccessA` and `FailureA` are the types that contains the relevant information regarding its success or failure.
     * The activity `A` must overload a no argument `operator()()` which initiates the activity. 
     * After the activity is initiated either `success()` or `failure()` methods must be called in order to signal its completion.
     * The activity `A` must take the collector as the first argument to its constructor, which is passed to the base class `activity<A, SuccessA, FailureA>`.
     * Hence its prefered to take the first parameter to the constructor as template parameter.
     * 
     * \tparam DerivedT Activity Class 
     * \tparam SuccessDataT data associated to the activity if the activity succeeds
     * \tparam FailureDataT data associated to the activity if the activity fails
     * 
     * \ingroup activities
     */
    template <typename DerivedT, typename SuccessDataT = void, typename FailureDataT = void>
    struct activity: std::enable_shared_from_this<DerivedT>, udho::activities::result<DerivedT, SuccessDataT, FailureDataT>{
        typedef std::shared_ptr<DerivedT> derived_ptr_type;
        
        template <typename StoreT>
        activity(StoreT& store): udho::activities::result<DerivedT, SuccessDataT, FailureDataT>(store){}
        
        /**
         * shared_ptr to this
         */
        derived_ptr_type self() {
            return std::enable_shared_from_this<DerivedT>::shared_from_this();
        }
    };
    
}

}

#endif // UDHO_ACTIVITIES_ACTIVITY_H

