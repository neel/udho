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

#ifndef UDHO_ACTIVITIES_ANALYZER_H
#define UDHO_ACTIVITIES_ANALYZER_H

#include <cstdint>
#include <ostream>
#include <udho/activities/states.h>
#include <udho/activities/accessor.h>
#include <udho/activities/result_data.h>

namespace udho{
/**
 * \ingroup activities
 */
namespace activities{
    
    /**
     * \code
     * const udho::accessor<A1, A2i, A3i>& d;
     * d.apply<A1>(udho::activities::analyzer<A1>()
     *      .canceled([](const A1SData& s, const A1FData& f){
     *          std::cout << "CANCELED" << std::endl;
     *      })
     *      .incomplete([](){
     *          std::cout << "INCOMPLETE" << std::endl;
     *      })
     *      .failure([](const A1FData& f){
     *          std::cout << "FAILED" << std::endl;
     *      })
     *      .success([](const A1SData& s){
     *          std::cout << "SUCCESS" << std::endl;
     *      })
     * );
     * \endcode
     * An analyzer can be applied on an activity to analyze its result data. The analyzer can be equipped with callbackss to handle different result states.
     * \ingroup activities
     * \ingroup data
     * \see state
     */
    template <typename ActivityT>
    class analyzer{
        typedef ActivityT activity_type;
        typedef typename activity_type::success_type success_type;
        typedef typename activity_type::failure_type failure_type;
        typedef boost::function<void ()> incomplete_ftor_type;
        typedef boost::function<void (const success_type&, const failure_type&)> cancelation_ftor_type;
        typedef boost::function<void (const failure_type&)> failure_ftor_type;
        typedef boost::function<void (const success_type&)> success_ftor_type;
        typedef analyzer<ActivityT> self_type;
        typedef state activity_state;

        friend struct result_data<success_type, failure_type>;
        
        accessor<ActivityT>   _accessor;
        incomplete_ftor_type  _incomplete;
        cancelation_ftor_type _canceled;
        failure_ftor_type     _failure;
        success_ftor_type     _success;
        success_ftor_type     _error;
        activity_state        _state;

     public:
        void operator()(){
            // either there is no error callback and there is an incomplete callback
            // or there is an error callback but the task has not been canceled
            if(_error.empty() || !_accessor.template canceled<ActivityT>()){
                if(!_incomplete.empty()) _incomplete();
                _state = state::incomplete;
            }
        }
        void operator()(const success_type& s, const failure_type& f){
            // there is a cancel callback then call that ignoring what other callback is set
            if(!_canceled.empty()){
                _canceled(s, f);
                _state = state::canceled;
            }else{
                if(!_accessor.template failed<ActivityT>()){ // succeeded but canceled, hence error 
                    if(!_error.empty()) _error(s);
                    _state = state::error;
                }else if(_accessor.template completed<ActivityT>()){ // failed then canceled, hence call failure callback instead
                    if(!_failure.empty()) _failure(f);
                    _state = state::failure;
                }else{ // incomplete, never invoked, skipped possibly because at least one of its dependency canceled
                    if(!_incomplete.empty()) _incomplete();
                    _state = state::incomplete;
                }
            }
        }
        void operator()(const failure_type& f){
            // either there is no error callback and there is a failure callback set
            // or there is an error callback but the task has not been canceled
            if(_error.empty() || !_accessor.template canceled<ActivityT>()){
                if(!_failure.empty()) _failure(f);
                _state = state::failure;
            }
        }
        void operator()(const success_type& s){
            if(!_success.empty()) _success(s);
            _state = state::success;
        }
        
        public:
            /**
             * construct an analyzer
             */
            template <typename AccessorT>
            analyzer(AccessorT accessor): _accessor(accessor), _state(state::unknown){}
            
            /**
             * If the activity is incomplete and an incomplete callback is set then the incomplete callback is invoked if any of the following holds
             * 
             * - there is no error callback 
             * - there is an error callback but the task has not been canceled
             * - there is an error callback and the task has been canceled (\see operator()(const success_type&, const failure_type&))
             */
            self_type& incomplete(incomplete_ftor_type ftor){
                _incomplete = ftor;
                return *this;
            }
            /**
             * If the activity is canceled and a canceled callback is set then the canceled callback is invoked if the following holds
             * 
             * - there is no error callback 
             */
            self_type& canceled(cancelation_ftor_type ftor){
                _canceled = ftor;
                return *this;
            }
            /**
             * The error callback is called if the subtask has not failed but has been canceled (through cancel_if)
             * Once the error callback is set failure and incomplete callbacks are invoked differently
             * The error callback is called if the following holds
             * 
             * - there is an error callback set and the activity is canceled and the activity has not failed
             */
            self_type& error(success_ftor_type ftor){
                _error = ftor;
                return *this;
            }
            /**
             * If the activity has failed and afailure callback is set then the failure callback is called if any of the following holds
             * 
             * - there is no error callback
             * - there is an error callback set and the activity is canceled 
             * - there is an error callback set and the activity is not canceled 
             */
            self_type& failure(failure_ftor_type ftor){
                _failure = ftor;
                return *this;
            }
            /**
             * If the activity is complete and neither failed nor canceled then the success callback is called if a a success callback is set
             */
            self_type& success(success_ftor_type ftor){
                _success = ftor;
                return *this;
            }
            
            activity_state apply(){
                _accessor.template apply<ActivityT>(std::ref(*this));
                return _state;
            }
            
            bool successful(){
                activity_state state = apply();
                return state == udho::activities::state::success;
            }
    };
    
}

}

#endif // UDHO_ACTIVITIES_ANALYZER_H

