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
#include <udho/activities/accessor.h>
#include <udho/activities/result_data.h>

namespace udho{
/**
 * @ingroup activities
 */
namespace activities{
    
    /**
     * @brief Completion handler for an activity.
     * @tparam SuccessT success data associated with the activity (requires default constructible)
     * @tparam FailureT failure data associated with the activity (requires default constructible)
     * @ingroup activities
     * States
     * |   Entry   |           |          |   Config  |     Hooks    |              |               |    State    |            |        |          |         | Exit      |
     * |:---------:|:---------:|:--------:|:---------:|:------------:|:------------:|:-------------:|:-----------:|:----------:|:------:|:--------:|:-------:|-----------|
     * | success() | failure() | cancel() | _required | _cancel_if() | _if_failed() | _if_errored() | completed() | canceled() | okay() | failed() | error() | Next      |
     * |           |           |     *    |    TRUE   |      N/A     |      N/A     |      N/A      |    FALSE    |    TRUE    |  FALSE |   FALSE  |  FALSE  | cancel()  |
     * |           |     *     |          |    TRUE   |      N/A     |    Not Set   |      N/A      |     TRUE    |    FALSE   |  FALSE |   TRUE   |  FALSE  | cancel()  |
     * |           |     *     |          |    TRUE   |      N/A     |     TRUE     |      N/A      |     TRUE    |    FALSE   |  FALSE |   TRUE   |  FALSE  | cancel()  |
     * |           |     *     |          |    TRUE   |      N/A     |     FALSE    |      N/A      |     TRUE    |    FALSE   |  FALSE |   TRUE   |  FALSE  | proceed() |
     * |     *     |           |          |    TRUE   |    Not Set   |      N/A     |      N/A      |     TRUE    |    FALSE   |  TRUE  |   FALSE  |  FALSE  | proceed() |
     * |     *     |           |          |    TRUE   |     FALSE    |      N/A     |      N/A      |     TRUE    |    FALSE   |  TRUE  |   FALSE  |  FALSE  | proceed() |
     * |     *     |           |          |    TRUE   |     TRUE     |      N/A     |    Not Set    |     TRUE    |    TRUE    |  TRUE  |   FALSE  |   TRUE  | cancel()  |
     * |     *     |           |          |    TRUE   |     TRUE     |      N/A     |      TRUE     |     TRUE    |    TRUE    |  TRUE  |   FALSE  |   TRUE  | cancel()  |
     * |     *     |           |          |    TRUE   |     TRUE     |      N/A     |     FALSE     |     TRUE    |    FALSE   |  TRUE  |   FALSE  |  FALSE  | proceed() |
     * |           |           |     *    |   FALSE   |      N/A     |      N/A     |      N/A      |    FALSE    |    TRUE    |  FALSE |   FALSE  |  FALSE  | cancel()  |
     * |           |     *     |          |   FALSE   |      N/A     |      N/A     |      N/A      |     TRUE    |    FALSE   |  FALSE |   TRUE   |  FALSE  | proceed() |
     * |     *     |           |          |   FALSE   |      N/A     |      N/A     |      N/A      |     TRUE    |    FALSE   |  TRUE  |   FALSE  |  FALSE  | proceed() |
     * 
     */
    template <typename DerivedT, typename SuccessT, typename FailureT>
    struct result: private udho::activities::result_data<SuccessT, FailureT>{
        typedef udho::activities::result_data<SuccessT, FailureT> result_type;
        typedef accessor<detail::labeled<DerivedT, result_type>> accessor_type;
        typedef typename result_type::success_type success_type;
        typedef typename result_type::failure_type failure_type;
        typedef boost::signals2::signal<void (const result_type&)> signal_type;
        typedef boost::signals2::signal<void ()> cancelation_signal_type;
        typedef std::function<bool (const success_type&)> cancel_if_ftor;
        typedef std::function<bool (const success_type&)> abort_error_ftor;
        typedef std::function<bool (const failure_type&)> abort_failure_ftor;

        /**
         * @param store collector
         */
        template <typename StoreT>
        result(StoreT& store): _accessor(store), _required(true){}
        
        /**
         * attach another subtask as done callback which will be executed once this subtask finishes
         * @param cmb next subtask
         */
        template <typename CombinatorT>
        void done(CombinatorT cmb){
            boost::function<void (const result_type&)> fnc([cmb](const result_type& data){
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
         * @param flag 
         */
        void required(bool flag){
            _required = flag;
        }
        
        /**
         * @brief Force cancelation of the activity even after it is successful to stop propagating to the next activities
         * @param f callback of type `bool (const success_type&)` which should return true to cancel
         */
        void cancel_if(cancel_if_ftor f){
            _cancel_if = f;
        }
        /**
         * @brief Even if the activity runs successfully, it may be considered as error based on the response  
         * @param ftor callback of type `bool (const success_type&)` which should return true in order to cancel all child activities
         */
        void if_errored(abort_error_ftor ftor){
            _if_errored = ftor;
        }
        /**
         * @brief The supplied callback is called if the activity fails
         * @param ftor callback of type `bool (const failure_type&)` which should return true in order to cancel all child activities
         */
        void if_failed(abort_failure_ftor ftor){
            _if_failed = ftor;
        }
        /**
         * Mark the data as cancelled then propagate the cancellation accross all child activities.
         * If the current activity fails and there is an if_failed callback set then whether it propagates the cancellation or not is decided by the if_failed callback.
         * Otherwise if there is an if_errored callback set then that is called and its boolean output is used to determine whether to propagate the cancellation or not.
         * If neither if_failed nor if_errored callback is set then the cancellation propagates to the child activities.
         */
        void cancel(){
            result_type::set_cancel(true);
            _finish();
        }
        protected:
            /**
             * signal successful completion of the activity with success data of type SuccessT
             * @param data success data
             */
            void success(const success_type& data){
                result_type::set_success(data);
                _finish();
            }
            /**
             * signal failed completion of the activity with failure data of type FailureT
             * @param data failure data
             */
            void failure(const failure_type& data){
                result_type::set_failure(data);
                _finish();
            }
        private:
            void _finish() {
                if(result_type::canceled()){
                    _abort();
                }else if(result_type::failed()){
                    if(_required && (!_if_failed || _if_failed(result_type::failure_data()))){
                        _abort();
                    }else{
                        _proceed();
                    }
                }else{ 
                    if(_required && (_cancel_if && _cancel_if(result_type::success_data()))){
                        if(!_if_errored || _if_errored(result_type::success_data())){
                            result_type::set_cancel(true);
                            _abort();
                        }else{
                            _proceed();
                        }
                    }else{
                        _proceed();
                    }
                }
            }
            void _proceed() {
                _save();
                _signal(_data());
            }
            void _abort(){
                _save();
                _cancelation_signals();
            }
            void _save(){
                result_type self = _data();
                detail::labeled<DerivedT, result_type> labeled(self);
                _accessor << labeled;
            }
            const result_type& _data() const {
                return static_cast<const result_type&>(*this);
            }
        private:
            accessor_type           _accessor;
            signal_type             _signal;
            bool                    _required;
            cancelation_signal_type _cancelation_signals;
            cancel_if_ftor          _cancel_if;
            abort_error_ftor        _if_errored;
            abort_failure_ftor      _if_failed;
    };
    
}

}

#endif // UDHO_ACTIVITIES_RESULT_H

