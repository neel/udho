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
    
#ifndef __DOXYGEN__

namespace detail{
    template <bool invocable, typename FunctorT, typename... TargetsT>
    struct apply_helper_internal{
        void operator()(FunctorT& f, const TargetsT&... t){f(t...);}
    };
    template <typename FunctorT, typename... TargetsT>
    struct apply_helper_internal<false, FunctorT, TargetsT...>{
        void operator()(FunctorT&, const TargetsT&...){}
    };
    template <typename FunctorT, typename... TargetsT>
    using apply_helper_ = apply_helper_internal<udho::util::is_invocable<FunctorT, TargetsT...>::value, FunctorT, TargetsT... >;

    template <typename FunctorT, typename SuccessT, typename FailureT>
    struct apply_helper: detail::apply_helper_<FunctorT>, detail::apply_helper_<FunctorT, SuccessT>, detail::apply_helper_<FunctorT, FailureT>, detail::apply_helper_<FunctorT, SuccessT, FailureT>{
        FunctorT& _ftor;
        
        apply_helper(FunctorT& f): _ftor(f){}
        
        void operator()(){
            detail::apply_helper_<FunctorT>::operator()(_ftor);
        }
        void operator()(const SuccessT& s){
            detail::apply_helper_<FunctorT, SuccessT>::operator()(_ftor, s);
        }
        void operator()(const FailureT& f){
            detail::apply_helper_<FunctorT, FailureT>::operator()(_ftor, f);
        }
        void operator()(const SuccessT& s, const FailureT& f){
            detail::apply_helper_<FunctorT, SuccessT, FailureT>::operator()(_ftor, s, f);
        }
        
    };
}

/**
 * @brief Contains Copiable Success or Failure data for an activity.
 * Stores the resulting data of an activity, which may succedd or fail. The provided SuccessT and FailureT is used to store the success and failure data respectively.
 * result_data is not supposed to be used directly. Instead it is privately inherited by the @ref activities::result to store the activity results.
 * result_data provides three protected methods to store the activity result as shown below.
 * - set_success(const SuccessT&)
 * - set_failure(const FailureT&)
 * - set_cancel(bool)
 * .
 * Both set_success and set_failure methods mark the result_data as complete.
 * Success and failure data can be retrieved though the following methods method.
 * - success_data()
 * - failure_data()
 * .
 * The following 5 methods are provided to descrive the state of an activity.
 * - completed() 
 * - canceled()
 * - failed()
 * - okay()
 * - error()
 * .
 *
 * ### States 
 *
 * State of an activity is expressed as combination of three inputs.
 * - <b>_completed</b>: Initialized as false. Whenever a success of failure value is set _completed is set to true.
 * - <b>_canceled</b>:  Initialized as false. Set to true whenever an activity is canceled either explicitely or triggered by failure or cancalation of its parent activity.
 * - <b>_success</b>:   Initialized as false. Whenever a success value is set through set_success() method then the _success is set to true. When a failure value is set through the set_failure() method then the _success is set to false.
 * .
 * Based on these three input 5 methods are provided to get the information regarding the current state. 
 * Conditions of these methods returning true is shown in the following table.
 *
 * |             | _completed | _canceled | _success |      |
 * |-------------|------------|-----------|----------|------|
 * | completed() | TRUE       | N/A       | N/A      | TRUE |
 * | canceled()  | N/A        | TRUE      | N/A      | TRUE |
 * | okay()      | TRUE       | FALSE     | TRUE     | TRUE |
 * | failed()    | TRUE       | FALSE     | FALSE    | TRUE |
 * | error()     | TRUE       | TRUE      | TRUE     | TRUE |
 *
 * - completed(): returns true if _completed is set to true irrespective of value of other input variables.
 * - canceled():  returns true if _canceled is set to true irrespective of value of other input variables.
 * - failed():    returns true if _success is false. However setting _success to false after invocation (through set_failure() method) implies that the activity has completed. Additionally although a successful activity can be forcibly canceled though cancel_if() hook, a canceled activity cannot fail because it has never been executed. Hence failed() returns true only if _completed is true, _canceled is false and _success is false.  
 * - okay():      returns true if _success is true. However as succesful invocation implies complition, it is expected that if _success is true, _completed is also true. Hence okay() returns true only if _completed is true and _success is true.  
 * - error():     returns true for completed activities if both _success and _completed are true.
 * .
 * @tparam SuccessT success data type
 * @tparam FailureT failure data type
 * @note Both SuccessT and FailureT must be default constructible.
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
     */
    result_data(): _completed(false), _success(false), _canceled(false){}

    /**
     * @brief either success or failure data set
     * @note check exists<Activity>() before calling completed()
     */
    inline bool completed() const{
        return _completed;
    }
    /**
     * @brief whether the activity has failed.
     * @note an incomplete or canceled activity is neither okay() not failed()
     */
    inline bool failed() const{
        return _completed && !_canceled && !_success;
    }
    /**
     * @brief check whether the activity has been canceled
     */
    inline bool canceled() const{
        return _canceled;
    }
    /**
     * @brief 
     */
    inline bool error() const{
        return _completed && _success && _canceled;
    }
    /**
     * @brief whether the activity has successfully completed.
     * @note an incomplete or canceled activity is neither okay() not failed()
     */
    inline bool okay() const{
        return _completed && _success;
    }
    /**
     * @brief retrieve the success data 
     * @note call okay() before calling success_data
     */
    inline const success_type& success_data() const{
        return _sdata;
    }
    /**
     * @brief retrieve the failure data
     * @note call failed() before calling failure_data
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
        detail::apply_helper<CallableT, success_type, failure_type> helper(callback);
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
        void set_success(const success_type& data){
            _sdata     = data;
            _success   = true;
            _completed = true;
        }
        /**
         * Set Failure Data
         */
        void set_failure(const failure_type& data){
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
    
#endif // __DOXYGEN__

}
}

#endif // UDHO_ACTIVITIES_RESULT_DATA_H
