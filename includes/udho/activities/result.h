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

#include <udho/pretty/pretty.h>

namespace udho{
namespace activities{
    
#ifndef __DOXYGEN__

    /**
     * @brief activity result
     * @ref activities::activity uses @ref activities::result to provide storage for activity results and hooks to the activities behaviour.
     * The life of an activity is started when its operator()() is called without any arguments.
     * Then the activity needs to call either success() or failure() methods (derived from @ref result) in order signal its success or failure.
     * Depending on success or failure, the next activities that depend on the successful or failed activity is either executed or canceled.
     * Hence we can consider three different entry points for the evaluation of the activity data e.g. success(), failure() and cancel().
     * Similarly there can be two exit routes abort() or proceed() which cancels or executes the dependent activities respectively.
     * @ref result allows the user code to provide a set of callbacks to react and to specialize the exit route for different activity instances.
     * The provided callbacks are called on different scenarios with the provided success / failure data. 
     * The returned value of these callbacks may alter the predetermined exit route i.e. proceed an activity which was supposed to abort due to failure or vice versa.
     * These hooks not only provides a means of further analysis of the success or failure data, but also it provides a means to react to the success an failure events, on case by case basis.
     * Following is an example use case of these hooks.
     *
     * > For a database activity a successful query evaluation results to success. Only failed query invocation is considered as failed.
     * > However for certain queries an empty result may be considered as a failure is sprite of the query being evaluated successfully.
     * > Of course in such cases the activity itself may analyze the resultset and respond with failure instead responding with success.
     * > But for a generic database activity it may be difficult to distinguish between a valid empty resultset from an invalid one.
     * > In such scenarios these hooks provide an additional way to treat that successful query evaluation differently (by taking abort 
     * > exit route instead of proceed) because an empty resultset is invalid in that particular case. It may also react with an 404 Error
     * > page as a consequence.
     *
     * Following are the 3 methods that can be used to set callbacks.
     * - if_failed()
     *
     *   Generally if an activity fails then the abort exit route is taken and all dependent activities are canceled(). 
     *   However if a callback is set through if_failed() then that is called and its return value determines whether it should abort or proceed. 
     *   The provided callback is called with the faulre result and if it returns false then the dependent activities are not canceled, instead 
     *   it takes the proceed exit route. Otherwise, if the provided callback returns true then as usual all dependent activities are canceled.
     *
     * - cancel_if()
     *
     *  Generally if an activity yields success result then the proceed exit route is taken an all dependent activities are executed. However if 
     *  a callback is set through cancel_if() then that is called with the success result. If the callback returns true then the activity is canceled
     *  in spite of being successful and it takes the abort exit route which cancels all dependent activities.
     *
     * - if_errored()
     *
     *  If an activity that yielded success result has been suggested to canceled by cancel_if then the callback set by if_errored is called.
     *  If that callback is set to false then the previously decided abort route is abandoned and the proceed route is taken.
     * .
     *
     * The state of an activity is described through the following 5 methods.
     * - completed() 
     * - canceled()
     * - failed()
     * - okay()
     * - error()
     * .
     * The following table depicts how the entry and the hooks impact the state and the exit route of an activity invocation.
     *
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
     * @tparam SuccessT success data associated with the activity (requires default constructible)
     * @tparam FailureT failure data associated with the activity (requires default constructible)
     * @ingroup activities
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
            std::cout << "[activity] canceled " << udho::pretty::name<DerivedT>() << std::endl;
            result_type::set_cancel(true);
            _finish();
        }

        using result_type::completed;
        using result_type::failed;
        using result_type::canceled;
        using result_type::okay;
        
        protected:
            /**
             * signal successful completion of the activity with success data of type SuccessT
             * @param data success data
             */
            void success(const success_type& data){
                std::cout << "[activity] successfull " << udho::pretty::name<DerivedT>() << std::endl;
                result_type::set_success(data);
                _finish();
            }
            /**
             * signal failed completion of the activity with failure data of type FailureT
             * @param data failure data
             */
            void failure(const failure_type& data){
                std::cout << "[activity] failed " << udho::pretty::name<DerivedT>() << std::endl;
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
    
#endif // __DOXYGEN__

}
}

#endif // UDHO_ACTIVITIES_RESULT_H

