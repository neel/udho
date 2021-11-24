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

#ifndef UDHO_ACTIVITIES_STATES_H
#define UDHO_ACTIVITIES_STATES_H

#include <cstdint>
#include <ostream>

namespace udho{
/**
 * \ingroup activities
 */
namespace activities{
    
    /**
     * activity result states
     * \ingroup activities
     */
    enum class state{
        unknown,
        /**
         * The activity is incomplete
         */
        incomplete,
        /**
         * The activity has failed
         */
        failure,
        /**
         * The activity completed successfully
         */
        success,
        /**
         * The activity is canceled
         */
        canceled,
        /**
         * The activity is canceled but has not failed.
         * The error state is only used when an error callback is passed to the analyzer
         */
        error
    };
    
    inline std::ostream& operator<<(std::ostream& os, const state& s){
        static const char* state_labels[] = {"unknown", "incomplete", "failure", "success", "canceled", "error"};
        std::uint32_t state_idx = static_cast<std::uint32_t>(s);
        os << "(" << state_idx << ") " << state_labels[state_idx];
        return os;
    }
    
}

}

#endif // UDHO_ACTIVITIES_STATES_H

