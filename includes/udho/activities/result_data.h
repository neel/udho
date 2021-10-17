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

#ifndef UDHO_ACTIVITIES_RESULT_DATA_H
#define UDHO_ACTIVITIES_RESULT_DATA_H

#include <udho/util.h>

namespace udho{
namespace activities{
    
namespace detail{
    template <bool invocable, typename FunctorT, typename... TargetsT>
    struct apply_helper_{
        void operator()(FunctorT& f, const TargetsT&... t){f(t...);}
    };
    template <typename FunctorT, typename... TargetsT>
    struct apply_helper_<false, FunctorT, TargetsT...>{
        void operator()(FunctorT&, const TargetsT&...){}
    };
    template <typename FunctorT, typename... TargetsT>
    using apply_helper = apply_helper_<udho::util::is_invocable<FunctorT, TargetsT...>::value, FunctorT, TargetsT... >;
}

template <typename FunctorT, typename SuccessT, typename FailureT>
struct apply_helper: detail::apply_helper<FunctorT>, detail::apply_helper<FunctorT, SuccessT>, detail::apply_helper<FunctorT, FailureT>, detail::apply_helper<FunctorT, SuccessT, FailureT>{
    FunctorT& _ftor;
    
    apply_helper(FunctorT& f): _ftor(f){}
    
    void operator()(){
        detail::apply_helper<FunctorT>::operator()(_ftor);
    }
    void operator()(const SuccessT& s){
        detail::apply_helper<FunctorT, SuccessT>::operator()(_ftor, s);
    }
    void operator()(const FailureT& f){
        detail::apply_helper<FunctorT, FailureT>::operator()(_ftor, f);
    }
    void operator()(const SuccessT& s, const FailureT& f){
        detail::apply_helper<FunctorT, SuccessT, FailureT>::operator()(_ftor, s, f);
    }
    
};

/**
 * @brief Contains Copiable Success or Failure data for an activity.
 * Contains one SuccessT and one FailureT values using their default constructors. 
 * @note Both SuccessT and FailureT must be default constructible.
 * @tparam SuccessT success data type
 * @tparam FailureT failure data type
 * @ingroup activities
 * @ingroup data
 */
template <typename SuccessT, typename FailureT>
struct result_data{
    typedef SuccessT success_type;
    typedef FailureT failure_type;
    typedef result_data<SuccessT, FailureT> self_type;
    
    /**
     * @brief Construct a new result data object
     * 
     */
    result_data(): _completed(false), _success(false), _canceled(false){}

    /**
     * either success or failure data set
     */
    inline bool completed() const{
        return _completed;
    }
    /**
     * whether the activity has failed
     */
    inline bool failed() const{
        return !_success;
    }
    /**
     * check whether the activity has been canceled
     */
    inline bool canceled() const{
        return _canceled;
    }
    
    inline bool okay() const{
        return completed() && !failed() && !canceled();
    }
    /**
     * Success data 
     */
    inline const success_type& success_data() const{
        return _sdata;
    }
    /**
     * Failure data
     */
    inline const failure_type& failure_data() const{
        return _fdata;
    }
    
    /**
     * Apply a callable to the result data which will be invoked exactly once with appropriate arguments. 
     * The invocation of the callback depends on the state as shown below.
     * 
     * - Incomplete: callback()
     * - Canceled: callback(const SuccessT&, const FailureT&)
     * - Failed: callback(const FailureT&)
     * - Successful: callback(const SuccessT&)
     * 
     * The callback may not have all the overloads. 
     * 
     * @param callback 
     */
    template <typename CallableT>
    void apply(CallableT callback){
        apply_helper<CallableT, success_type, failure_type> helper(callback);
        if(!completed()){ // Never completed hence success or failure does not matter
            helper();
        }else if(canceled()){ // cancelation may occur after failure or after success (forced by cancel_if) hence passed both success and failure data as callback
            helper(success_data(), failure_data());
        }else{
            if(!failed()) helper(success_data()); // complete and successful 
            if(failed())  helper(failure_data()); // complete and failed
        }
    }
    protected:
        /**
         * Set Success Data
         */
        void success(const success_type& data){
            _sdata     = data;
            _success   = true;
            _completed = true;
        }
        /**
         * Set Failure Data
         */
        void failure(const failure_type& data){
            _fdata     = data;
            _success   = false;
            _completed = true;
        }
        /**
         * mark as canceled
         */
        void set_cancel(bool canceled = true){  
            _canceled = canceled;
        }
    private:
        bool _completed;
        bool _success;
        bool _canceled;
        success_type _sdata;
        failure_type _fdata;
};
    
}
}

#endif // UDHO_ACTIVITIES_RESULT_DATA_H
