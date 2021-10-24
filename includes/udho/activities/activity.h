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
namespace activities{
    
#ifndef __DOXYGEN__

    /**
     * An activity `A` must subclass from `activity<A, SuccessA, FailureA>` assuming `SuccessA` and `FailureA` are the types that contains the relevant information regarding its success or failure.
     * The activity `A` must overload a no argument `operator()()` which initiates the activity. 
     * After the activity is initiated either `success()` or `failure()` methods must be called in order to signal its completion.
     * The activity `A` must take the collector as the first argument to its constructor, which is passed to the base class `activity<A, SuccessA, FailureA>`.
     * Hence its prefered to take the first parameter to the constructor as template parameter.
     * 
     * @tparam DerivedT Activity Class 
     * @tparam SuccessDataT data associated to the activity if the activity succeeds
     * @tparam FailureDataT data associated to the activity if the activity fails
     * 
     * @ingroup activities
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
    
#else
    /**
     * @brief Activity
     * An activity consists of one or more asynchronous operations that yields success or failure data after completion.
     * An activity is defined in a class X that inherits from activity<X> and provides a no argument operator() overload  
     * to initiate the operation, which ultimately leads to a call to either of the success() or failure() functions.
     *
     * An activity also defines two datastructures that is supposed to store the result of successful and failed evaluations
     * as shown in the example below.
     * @include {} snippets/activities/minimal/data.h 
     * The activity must provide a no argument operator() overload which can be called publicly. Two examples of basic minimal 
     * activities are shown below.
     * @include {} snippets/activities/minimal/activity.h 
     * In the first example the first activity X always succeeds with a success value of 42 while the second activity Y always
     * fails with a failure value of 24. The above two are very simple example. However the activities are actually intended 
     * to perform asynchronous activities as shown the the next example.
     * @include {} snippets/activities/minimal/async_activity.h 
     * A concrete example is shown below in which the activity's operator() starts a timer that after 2 seconds calls its finish 
     * method which calls success() with the success value of 42.
     * @include {} snippets/activities/minimal/timer_activity.h 
     * Some tasks may requre a chain, rather a tree of activities to complete in order to derive the final conclusion. Some
     * activities may be dependent on others. So an activity graph can be constructed by linking a dependent activity with the 
     * parent activity. All these activities store their result (success or failure) into a common collector which is provided 
     * to the activities via its constructor. The activity constructor must pass that to it's base classes constructor. The 
     * collector contains the result_data of all activities on the activity graph. For example an activity graph consisting of
     * activities A1, A2 and A3 a collector like the following would be required.
     * @code {.cpp}
     * auto collector = activities::collect<A1, A2, A3>(ctx);
     * @endcode
     * The @ref activities::collect method returns a shared_pointer to a collector. This collector has to be passed to the 
     * constructors of all activities A1, A2 and A3. Generally not only the collector, but also the activities are instantiated
     * as shared pointers using std::make_shared. One activity is linked with another through a combinator as shown in the
     * following example. 
     * @include {} snippets/activities/minimal/combinator.cpp
     * To link A1 with A2 we create a `combinator<A2, A1>` and pass that combinator to the done() method of A1's shared_ptr.
     * A `combinator<Next, Dependencies...>` may depend on one or more other activities. It takes the next activity as the first  
     * template parameter. In the above example the activity A2 depends on the completion of A1. Hence A1 is provided as the 
     * second template parameter. If A2 depends on tw activities `A1`and `A0` then a `combinator<A2, A1, A0>` would be needed.
     *
     * Generally when an activity fails all the dependent activities are canceled. However such behaviours can be overridden
     * by hooks.
     *
     * ### Hooks
     * The life of an activity is started when its operator()() is called without any arguments.
     * Then the activity needs to call either success() or failure() methods in order signal its success or failure. Depending on success
     * or failure, the next activities that depend on the successful or failed activity is either executed or canceled. Hence we can
     * consider three different entry points for the evaluation of the activity data e.g. success(), failure() and cancel(). Similarly
     * there can be two exit routes abort() or proceed() which cancels or executes the dependent activities respectively.
     *
     * User code may provide a set of callbacks to react and to specialize the exit route for different activity instances. The provided 
     * callbacks are called on different scenarios with the provided success / failure data. The returned value of these callbacks may alter
     * the predetermined exit route i.e. proceed an activity which was supposed to abort due to failure or vice versa. These hooks not only 
     * provides a means of further analysis of the success or failure data, but also it provides a means to react to the success an failure 
     * events, on case by case basis. Following is an example use case of these hooks.
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
     *
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
     * ### States
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
     *
     * @tparam DerivedT 
     * @tparam SuccessDataT 
     * @tparam FailureDataT 
     */
    template <typename DerivedT, typename SuccessDataT, typename FailureDataT>
    struct activity: std::enable_shared_from_this<DerivedT>{
        template <typename StoreT>
        activity(StoreT& store);

        std::shared_ptr<DerivedT> self() {
            return std::enable_shared_from_this<DerivedT>::shared_from_this();
        }
        /**
         * mark the activity as required or optional
         * @param flag 
         */
        void required(bool flag){
            _required = flag;
        }
        /**
         * mark the activity as required or optional
         * @param flag 
         */
        void required(bool flag){
            _required = flag;
        }
        
        /// @name hooks
        /// @{
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
        /// @}
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
        /// @name states
        /// @{
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
        /// @}
        /// @name data
        /// @{ 
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
        /// @}
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
    };
#endif // __DOXYGEN__

}

}

#endif // UDHO_ACTIVITIES_ACTIVITY_H

