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

#ifndef UDHO_ACTIVITIES_CONVENIANCE_H
#define UDHO_ACTIVITIES_CONVENIANCE_H

#include <string>
#include <udho/activities/activity.h>
#include <udho/activities/perform.h>
#include <udho/activities/require.h>
#include <udho/activities/analyzer.h>
#include <udho/activities/after.h>

namespace udho{

/**
 * shorthand for udho::activities::collect
 * \ingroup data
 */
template <typename... T, typename ContextT>
auto collect(ContextT& ctx){
    return udho::activities::collect<T...>(ctx);
}

/**
 * shorthand for udho::activities::accessor
 * \see udho::activities::accessor
 * \ingroup data
 */
template <typename... T>
using accessor = udho::activities::accessor<T...>;

/**
 * shorthand for udho::activities::activity
 * \see udho::activities::activity
 * \ingroup activities
 */
template <typename DerivedT, typename SuccessDataT, typename FailureDataT>
using activity = udho::activities::activity<DerivedT, SuccessDataT, FailureDataT>;

/**
 * shorthand for udho::activities::require
 * \see udho::activities::require
 * \ingroup activities
 */
template <typename... DependenciesT>
using require = udho::activities::require<DependenciesT...>;

/**
 * shorthand for udho::activities::perform
 * \see udho::activities::perform
 * \ingroup activities
 */
template <typename ActivityT>
using perform = udho::activities::perform<ActivityT>;

/**
 * \see udho::activities::analyzer
 * \ingroup activities
 */
template <typename ActivityT, typename AccessorT>
udho::activities::analyzer<ActivityT> analyze(AccessorT& accessor){
    return udho::activities::analyzer<ActivityT>(accessor);
}
/**
 * @brief 
 * 
 * @tparam T 
 * @param dependencies 
 * @return udho::activities::after<T...> 
 */
template <typename... T>
udho::activities::basic_after<udho::activities::subtask, T...> after(T&... dependencies){
    return udho::activities::after<T...>(dependencies...);
}

inline udho::activities::after_none after(){
    return udho::activities::after_none();
}

}

#endif // UDHO_ACTIVITIES_CONVENIANCE_H

