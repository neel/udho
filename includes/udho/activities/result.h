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

#ifndef UDHO_ACTIVITIES_RESULT_H
#define UDHO_ACTIVITIES_RESULT_H

#include <cstdint>
#include <ostream>
#include <boost/signals2.hpp>
#include <udho/activities/data.h>
#include <udho/activities/result_data.h>

namespace udho{
/**
 * \ingroup activities
 */
namespace activities{
    
    /**
     * Completion handler for an activity.
     * \tparam SuccessT success data associated with the activity 
     * \tparam FailureT failure data associated with the activity
     * 
     * \ingroup activities
     */
    template <typename DerivedT, typename SuccessT, typename FailureT>
    struct result: result_data<SuccessT, FailureT>{
        typedef result_data<SuccessT, FailureT> data_type;
        typedef accessor<detail::labeled<DerivedT, data_type>> accessor_type;
        typedef typename data_type::success_type success_type;
        typedef typename data_type::failure_type failure_type;
        typedef boost::signals2::signal<void (const data_type&)> signal_type;
        typedef boost::signals2::signal<void ()> cancelation_signal_type;
        typedef boost::function<bool (const success_type&)> cancel_if_ftor;
        typedef boost::function<bool (const success_type&)> abort_error_ftor;
        typedef boost::function<bool (const failure_type&)> abort_failure_ftor;
        
        template <typename NextT, typename... DependenciesT>
        friend struct combinator;
        
        accessor_type _shadow;
        signal_type   _signal;
        bool          _required;
        cancelation_signal_type _cancelation_signals;
        cancel_if_ftor _cancel_if;
        abort_error_ftor _abort_error;
        abort_failure_ftor _abort_failure;
        
        /**
         * \param store collector
         */
        template <typename StoreT>
        result(StoreT& store): _shadow(store), _required(true){}
        
        /**
         * attach another subtask as done callback which will be executed once this subtask finishes
         * \param cmb next subtask
         */
        template <typename CombinatorT>
        void done(CombinatorT cmb){
            boost::function<void (const data_type&)> fnc([cmb](const data_type& data){
                cmb->operator()(data);
            });
            _signal.connect(fnc);
            boost::function<void ()> canceler([cmb](){
                cmb->cancel();
            });
            _cancelation_signals.connect(canceler);
        }
        
        /**
         * mark the activity as required or optional
         * \param flag 
         */
        void required(bool flag){
            _required = flag;
        }
        
        /**
         * Force cancelation of the activity even after it is successful to stop propagating to the next activities
         * \param f callback which should return true to signal cancelation
         */
        void cancel_if(cancel_if_ftor f){
            _cancel_if = f;
        }
        void if_errored(abort_error_ftor ftor){
            _abort_error = ftor;
        }
        void if_failed(abort_failure_ftor ftor){
            _abort_failure = ftor;
        }
        protected:
            /**
             * signal successful completion of the activity with success data of type SuccessT
             * \param data success data
             */
            void success(const success_type& data){
                data_type::success(data);
                completed();
            }
            /**
             * signal failed completion of the activity with failure data of type FailureT
             * \param data failure data
             */
            void failure(const failure_type& data){
                data_type::failure(data);
                completed();
            }
        private:
            /**
             * Mark the data as cancelled then propagate the cancellation accross all child activities.
             * If the current activity fails and there is an if_failed callback set then whether it propagates the cancellation or not is decided by the if_failed callback.
             * Otherwise if there is an if_errored callback set then that is called and its boolean output is used to determine whether to propagate the cancellation or not.
             * If neither if_failed nor if_errored callback is set then the cancellation propagates to the child activities.
             */
            void cancel(){
                data_type::cancel();
                bool propagate = true;
                if(data_type::failed() && !_abort_failure.empty()){
                    propagate = _abort_failure(data_type::failure_data());
                }else if(!_abort_error.empty()){
                    propagate = _abort_error(data_type::success_data());
                }
                if(propagate) _cancelation_signals();
            }
            /**
             * Called after success/failure to execute the child activities. 
             * Stores the data of this activity.
             * Cancels if the current activity is required and has failed.
             * If there is a cancel_if callback set then this behavour is overridden by the boolean output of the provided callback.
             * Otherwise it continues executing the activity tree 
             */
            void completed(){                
                bool should_cancel = false;
                if(!data_type::failed()){
                    if(!_cancel_if.empty()){
                        should_cancel = _cancel_if(data_type::success_data());
                    }
                }else{
                    should_cancel = data_type::failed() && _required;
                }

                if(should_cancel) data_type::cancel();

                data_type self = static_cast<const data_type&>(*this);
                detail::labeled<DerivedT, data_type> labeled(self);
                _shadow << labeled;
                
                if(should_cancel){
                    cancel();
                }else{
                    _signal(self);
                }
            }
    };
    
}

}

#endif // UDHO_ACTIVITIES_RESULT_H

