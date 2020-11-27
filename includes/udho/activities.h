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

#ifndef ACTIVITIES_H
#define ACTIVITIES_H

#include <mutex>
#include <string>
#include <atomic>
#include <boost/signals2.hpp>
#include <udho/cache.h>
#include <boost/bind.hpp>
#include <udho/util.h>

namespace udho{
/**
 * \ingroup activities
 */
namespace activities{
    /**
     * \defgroup data
     * \ingroup activities
     */    
    template <typename StoreT>
    struct fixed_key_accessor{
        typedef typename StoreT::key_type key_type;
        
        StoreT&  _shadow;
        key_type _key;
        
        fixed_key_accessor(StoreT& store, const key_type& key): _shadow(store), _key(key){}
        std::string key() const{ return _key; }        
        
        template <typename V>
        bool exists() const{
            return _shadow.template exists<V>(key());
        }
        template <typename V>
        V get() const{
            return _shadow.template get<V>(key());
        }
        template <typename V>
        V at(){
            return _shadow.template at<V>(key());
        }
        template <typename V>
        void set(const V& value){
            _shadow.template set<V>(key(), value);
        }
        std::size_t size() const{
            return _shadow.size();
        }
    };
       
    template <typename... T>
    struct accessor;
    
    /**
     * Collects data associated with all activities involved in the subtask graph
     * \ingroup data
     */
    template <typename... T>
    struct collector: fixed_key_accessor<udho::cache::shadow<std::string, typename T::result_type...>>, std::enable_shared_from_this<collector<T...>>{
        typedef fixed_key_accessor<udho::cache::shadow<std::string, typename T::result_type...>> base_type;
        typedef udho::cache::store<udho::cache::storage::memory, std::string, typename T::result_type...> store_type;
        typedef typename store_type::shadow_type shadow_type;
        typedef accessor<T...> accessor_type;
        
        store_type  _store;
        shadow_type _shadow;
        std::string _name;
        
        collector(const udho::configuration_type& config, const std::string& name): base_type(_shadow, name), _store(config), _shadow(_store), _name(name){}
        std::string name() const{ return _name; }
        shadow_type& shadow() { return _shadow; }
        const shadow_type& shadow() const { return _shadow; }
    };
    
    /**
     * Access a subset of data from the collector
     * \ingroup data
     */
    template <typename... T>
    struct accessor: fixed_key_accessor<udho::cache::shadow<std::string, typename T::result_type...>>{
        typedef fixed_key_accessor<udho::cache::shadow<std::string, typename T::result_type...>> base_type;
        typedef udho::cache::shadow<std::string, typename T::result_type...> shadow_type;
        
        shadow_type _shadow;
        
        template <typename... U>
        accessor(std::shared_ptr<collector<U...>> collector): base_type(_shadow, collector->name()), _shadow(collector->shadow()){}
        template <typename... U>
        accessor(accessor<U...> accessor): base_type(_shadow, accessor.name()), _shadow(accessor.shadow()){}
        std::string name() const{ return base_type::key(); }
        shadow_type& shadow() { return _shadow; }
        const shadow_type& shadow() const { return _shadow; }
        
        /**
         * Whether there exists any data for activity V
         * \tparam V Activity Type
         */
        template <typename V>
        bool exists() const{
            return base_type::template exists<typename V::result_type>();
        }
        /**
         * get data associated with activity V
         * 
         * \tparam V activity type
         */
        template <typename V>
        const typename V::result_type& get() const{
            return base_type::template get<typename V::result_type>();
        }
        /**
         * Check whether activity V has completed.
         * \tparam V activity type
         */
        template <typename V>
        bool completed() const{
            if(base_type::template exists<typename V::result_type>()){
                typename V::result_type res = base_type::template get<typename V::result_type>();
                return res.completed();
            }
            return false;
        }
        /**
         * Check whether activity V has been canceled.
         * \tparam V activity type
         */
        template <typename V>
        bool canceled() const{
            if(base_type::template exists<typename V::result_type>()){
                typename V::result_type res = base_type::template get<typename V::result_type>();
                return res.canceled();
            }
            return false;
        }
        /**
         * Check whether activity V has failed (only the failure data of V is valid).
         * \tparam V activity type
         */
        template <typename V>
        bool failed() const{
            if(base_type::template exists<typename V::result_type>()){
                typename V::result_type res = base_type::template get<typename V::result_type>();
                return res.failed();
            }
            return true;
        }
        /**
         * Check whether activity V is okay.
         * \tparam V activity type
         */
        template <typename V>
        bool okay() const{
            if(base_type::template exists<typename V::result_type>()){
                typename V::result_type res = base_type::template get<typename V::result_type>();
                return res.okay();
            }
            return true;
        }
        /**
         * get success data for activity V
         * \tparam V activity type
         */
        template <typename V>
        typename V::result_type::success_type success() const{
            if(base_type::template exists<typename V::result_type>()){
                typename V::result_type res = base_type::template get<typename V::result_type>();
                return res.success_data();
            }
            return typename V::result_type::success_type();
        }
        /**
         * get failure data for activity V
         * \tparam V activity type
         */
        template <typename V>
        typename V::result_type::failure_type failure() const{
            if(base_type::template exists<typename V::result_type>()){
                typename V::result_type res = base_type::template get<typename V::result_type>();
                return res.failure_data();
            }
            return typename V::result_type::failure_type();
        }
        template <typename V>
        void set(const typename V::result_type& value){
            base_type::template set<typename V::result_type>(value);
        }
        /**
         * Apply a callback on result of V
         * \tparam V activity type
         * \param f callback
         */
        template <typename V, typename F>
        void apply(F f) const{
            if(base_type::template exists<typename V::result_type>()){
                typename V::result_type res = base_type::template get<typename V::result_type>();
                res.template apply<F>(f);
            }
        }
    };
    
    /**
     * \ingroup data
     */
    template <typename U, typename... T>
    collector<T...>& operator<<(collector<T...>& h, const U& data){
        auto& shadow = h.shadow();
        shadow.template set<U>(h.name(), data);
        return h;
    }

    /**
     * \ingroup data
     */
    template <typename U, typename... T>
    const collector<T...>& operator>>(const collector<T...>& h, U& data){
        const auto& shadow = h.shadow();
        data = shadow.template get<U>(h.name());
        return h;
    }
    
    /**
     * \ingroup data
     */
    template <typename U, typename... T>
    accessor<T...>& operator<<(accessor<T...>& h, const U& data){
        auto& shadow = h.shadow();
        shadow.template set<U>(h.name(), data);
        return h;
    }

    /**
     * \ingroup data
     */
    template <typename U, typename... T>
    const accessor<T...>& operator>>(const accessor<T...>& h, U& data){
        auto& shadow = h.shadow();
        data = shadow.template get<U>(h.name());
        return h;
    }
    
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
     * Contains **Copiable** Success or Failure data for an activity.
     * \tparam SuccessT success data type
     * \tparam FailureT failure data type
     * 
     * \ingroup activities
     * \ingroup data
     */
    template <typename SuccessT, typename FailureT>
    struct result_data{
        typedef SuccessT success_type;
        typedef FailureT failure_type;
        typedef result_data<SuccessT, FailureT> self_type;
        typedef self_type result_type;
        
        bool _completed;
        bool _success;
        bool _canceled;
        success_type _sdata;
        failure_type _fdata;
        
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
         * \param callback 
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
            void cancel(){
                _canceled = true;
            }
    };
    
    /**
     * activity result states
     * \ingroup activities
     * \ingroup data
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
        
//        template <bool invocable, typename FunctorT, typename... TargetsT>
//        friend struct detail::apply_helper_;

//        template <typename F, typename... Args>
//        friend struct udho::util::is_invocable;

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
    
    template <typename NextT, typename... DependenciesT>
    struct combinator;
    
    /**
     * Completion handler for an activity.
     * \tparam SuccessT success data associated with the activity 
     * \tparam FailureT failure data associated with the activity
     * 
     * \ingroup activities
     */
    template <typename SuccessT, typename FailureT>
    struct result: result_data<SuccessT, FailureT>{
        typedef result_data<SuccessT, FailureT> data_type;
        typedef accessor<data_type> accessor_type;
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
                _shadow << self;
                
                if(should_cancel){
                    cancel();
                }else{
                    _signal(self);
                }
            }
    };

    /**
     * \internal 
     */
    template <typename DependencyT>
    struct junction{
        void operator()(const DependencyT&){}
    };
    
    /**
     * A combinator combines multiple activities and proceeds towards the next activity
     * \tparam NextT next activity
     * \tparam DependenciesT dependencies
     * \ingroup activities
     */
    template <typename NextT, typename... DependenciesT>
    struct combinator: junction<typename DependenciesT::result_type>...{
        typedef std::shared_ptr<NextT> next_type;
        typedef boost::signals2::signal<void (NextT&)> signal_type;
        
        next_type  _next;
        std::atomic<std::size_t> _counter;
        std::mutex  _mutex;
        signal_type _preparators;
        std::atomic<bool> _canceled;
        
        combinator(next_type& next): _next(next), _counter(sizeof...(DependenciesT)), _canceled(false){}
        
        /**
         * whenever a subtask finishes the `operator()` of the combinator is called. which doesn't start the next subtask untill all the dependencies have completed.
         * Before starting the next activity the next activity is prepared if any preparator is passed through the `prepare()` function
         */
        template <typename U>
        void operator()(const U& u){
            junction<U>::operator()(u);
            propagate();
        }
        
        void cancel(){
            _canceled = true;
            propagate();
        }
        
        void propagate(){
            _counter--;
            if(!_counter){
                _mutex.lock();
                if(_canceled){
                    _next->cancel();
                }else{
                    if(!_preparators.empty()){
                        _preparators(*_next);
                        _preparators.disconnect_all_slots();
                    }
                    (*_next)();
                }
                _mutex.unlock();
            }
        }
        
        /**
         *set a preparator callback which will be called with a reference to teh next activity. The preparator callback is supposed to prepare the next activity by using the data callected till that time.
         */
        template <typename PreparatorT>
        void prepare(PreparatorT prep){
            boost::function<void (NextT&)> fnc(prep);
            _preparators.connect(prep);
        }
    };
       
    /**
     * A `subtask` is an instantiation of an `activity`. The subtask reuses an activity to model different use cases by attaching dependencies.
     * A subtask contains two shared pointers, one to the activity and another one to the combinator.
     * The subtask cannot be instantiated directly by calling the subtask constructor. Instead call the static `with` method to instantiate.
     * 
     * \tparam ActivityT The  activity 
     * \tparam DependenciesT The activities that has to be performed before performing ActivityT
     * \ingroup activities
     */
    template <typename ActivityT, typename... DependenciesT>
    struct subtask{
        typedef ActivityT activity_type;
        typedef combinator<ActivityT, DependenciesT...> combinator_type;
        typedef subtask<ActivityT, DependenciesT...> self_type;
        
        template <typename U, typename... DependenciesU>
        friend struct subtask;
        
        subtask(const self_type& other): _activity(other._activity), _combinator(other._combinator){}
        
        /**
         * shared pointer to the activity
         */
        std::shared_ptr<activity_type> activity() {
            return _activity;
        }
        
        /**
         * execute task next after the current one
         * \param next the next subtask
         */
        template <typename V, typename... DependenciesV>
        self_type& done(subtask<V, DependenciesV...>& next){
            activity()->done(next._combinator);
            return *this;
        }
        
        /**
         * t2.after(t1) is equivalent to t1.done(t2)
         * \param previous the previous subtask
         */
        template <typename V, typename... DependenciesV>
        self_type& after(subtask<V, DependenciesV...>& previous){
            previous._activity->done(_combinator);
            return *this;
        }
        
        /**
         * Arguments for the constructor of the Activity
         */
        template <typename... U>
        static self_type with(U&&... u){
            return self_type(0, u...);
        }
        
        /**
         * attach a callback which will be called with a reference to the activity after it has been instantiated and all its dependencies have completed.
         */
        template <typename PreparatorT>
        self_type& prepare(PreparatorT prep){
            _combinator->prepare(prep);
            return *this;
        }

        /**
         * Set required flag on or off. If a required subtask fails then all intermediate subtask that depend on it fails and the final callback is called immediately. By default all subtasks are required
         */
        self_type& required(bool flag){
            _activity->required(flag);
            return *this;
        }
        
        /**
         * Force cancelation of the activity even after it is successful to stop propagating to the next activities
         * \param f callback which should return true to signal cancelation
         */
        self_type& cancel_if(typename activity_type::cancel_if_ftor cancelor){
            _activity->cancel_if(cancelor);
            return *this;
        }
        
        /**
         * returns the shared pointer to the actiivity
         */
        std::shared_ptr<activity_type> operator->(){
            return _activity;
        }
        
        /**
         * abort if canceled if ftor returns false. f will be called with the success if it has been canceled due to error
         */
        template <typename FunctionT>        
        self_type& if_errored(FunctionT ftor){
            _activity->if_errored(ftor);
            return *this;
        }
        
        /**
         * abort if canceled if ftor returns false. f will be called with the failue data if it has been canceled due to failure
         */
        template <typename FunctionT>
        self_type& if_failed(FunctionT ftor){
            _activity->if_failed(ftor);
            return *this;
        }
        
        template <typename FunctionT>
        self_type& if_canceled(FunctionT ftor){
            typename activity_type::abort_error_ftor   ef = ftor;
            typename activity_type::abort_failure_ftor ff = ftor;
            if_errored(ef);
            if_failed(ff);
            return *this;
        }
        
        private:
            template <typename... U>
            subtask(int, U&&... u){
                _activity = std::make_shared<activity_type>(u...);
                _combinator = std::make_shared<combinator_type>(_activity);
            }
            
            std::shared_ptr<activity_type> _activity;
            std::shared_ptr<combinator_type> _combinator;
    };
    
    /**
     * Spetialization for the root subtask in the task graph
     * 
     * \tparam ActivityT The  activity 
     * \ingroup activities
     */
    template <typename ActivityT>
    struct subtask<ActivityT>{
        typedef ActivityT activity_type;
        typedef subtask<ActivityT> self_type;
        
        template <typename U, typename... DependenciesU>
        friend struct subtask;
        
        subtask(const self_type& other): _activity(other._activity){}
                
        std::shared_ptr<activity_type> activity() {
            return _activity;
        }
        
        /**
         * execute task next after the current one
         * \param next the next subtask
         */
        template <typename V, typename... DependenciesV>
        self_type& done(subtask<V, DependenciesV...>& next){
            activity()->done(next._combinator);
            return *this;
        }
        
        /**
         * Arguments for the constructor of the Activity
         */
        template <typename... U>
        static self_type with(U&&... u){
            return self_type(0, u...);
        }
        
        /**
         * calls the `operator()()` of the activity and starts executing the graph
         */
        template <typename... U>
        void operator()(U&&... u){
            _activity->operator()(u...);
        }
        
        /**
         * Set required flag on or off. If a required subtask fails then all intermediate subtask that depend on it fails and the final callback is called immediately. By default all subtasks are required
         */
        self_type& required(bool flag){
            _activity->required(flag);
            return *this;
        }
        
        /**
         * Force cancelation of the activity even after it is successful to stop propagating to the next activities
         * \param f callback which should return true to signal cancelation
         */
        self_type& cancel_if(typename activity_type::cancel_if_ftor cancelor){
            _activity->cancel_if(cancelor);
            return *this;
        }
        
        /**
         * returns the shared pointer to the actiivity
         */
        std::shared_ptr<activity_type> operator->(){
            return _activity;
        }
        
        /**
         * abort if canceled if ftor returns false. f will be called with the success if it has been canceled due to error
         */
        template <typename FunctionT>        
        self_type& if_errored(FunctionT ftor){
            _activity->if_errored(ftor);
            return *this;
        }
        
        /**
         * abort if canceled if ftor returns false. f will be called with the failue data if it has been canceled due to failure
         */
        template <typename FunctionT>
        self_type& if_failed(FunctionT ftor){
            _activity->if_failed(ftor);
            return *this;
        }
        
        template <typename FunctionT>
        self_type& if_canceled(FunctionT ftor){
            typename activity_type::abort_error_ftor   ef = ftor;
            typename activity_type::abort_failure_ftor ff = ftor;
            if_errored(ef);
            if_failed(ff);
            return *this;
        }
        
        private:
            template <typename... U>
            subtask(int, U&&... u){
                _activity = std::make_shared<activity_type>(u...);
            }
            
            std::shared_ptr<activity_type> _activity;
    };
    
    
    /**
     * create a subtask to perform activity ActivityT
     * \tparam ActivityT the activity to perform
     * \ingroup activities
     */
    template <typename ActivityT>
    struct perform{
        /**
         * mention the activities that has to be performed before executing this subtask.
         * \tparam DependenciesT dependencies
         */
        template <typename... DependenciesT>
        struct require{
            /**
             * arguments for the activity constructor
             */
            template <typename... U>
            static subtask<ActivityT, DependenciesT...> with(U&&... u){
                return subtask<ActivityT, DependenciesT...>::with(u...);
            }
        };
        
       /**
        * arguments for the activity constructor
        */
        template <typename... U>
        static subtask<ActivityT> with(U&&... u){
            return subtask<ActivityT>::with(u...);
        }
    };
    
    /**
     * \ingroup data
     */
    template <typename... T, typename ContextT>
    std::shared_ptr<collector<T...>> collect(ContextT& ctx, const std::string& name){
        return std::make_shared<collector<T...>>(ctx.aux().config(), name);
    }
    
    template <typename CallbackT, typename CollectorT>
    struct joined;
    
    /**
     * \ingroup activities
     */
    template <typename CallbackT, typename... T>
    struct joined<CallbackT, activities::collector<T...>>{
        typedef activities::collector<T...> collector_type;
        typedef typename collector_type::accessor_type accessor_type;
        typedef CallbackT callback_type;
        typedef joined<callback_type, activities::collector<T...>> self_type;
        typedef int cancel_if_ftor;
        
        joined(std::shared_ptr<collector_type> collector, CallbackT callback): _collector(collector), _callback(callback){}
        void operator()(){
            accessor_type accessor(_collector);
            _callback(accessor);
        }
        void cancel(){
            operator()();
        }
        template <typename U>
        void cancel_if(U&){}
        private:
            std::shared_ptr<collector_type> _collector;
            callback_type _callback;
    };
    
    namespace detail{
        template <typename CollectorT, typename... DependenciesT>
        struct final_intermediate{
            std::shared_ptr<CollectorT> _collector;
            
            final_intermediate(std::shared_ptr<CollectorT> collector): _collector(collector){}
            
            template <typename CallbackT>
            subtask<joined<CallbackT, CollectorT>, DependenciesT...> exec(CallbackT callback){
                return subtask<joined<CallbackT, CollectorT>, DependenciesT...>::with(_collector, callback);
            }
        };
    }
    
    /**
     * \ingroup activities
     */
    template <typename... DependenciesT>
    struct require{
        template <typename CollectorT>
        static detail::final_intermediate<CollectorT, DependenciesT...> with(std::shared_ptr<CollectorT> collector){
            return detail::final_intermediate<CollectorT, DependenciesT...>(collector);
        }
    };
    
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
    struct activity: std::enable_shared_from_this<DerivedT>, udho::activities::result<SuccessDataT, FailureDataT>{
        typedef std::shared_ptr<DerivedT> derived_ptr_type;
        
        template <typename StoreT>
        activity(StoreT& store): udho::activities::result<SuccessDataT, FailureDataT>(store){}
        
        /**
         * shared_ptr to this
         */
        derived_ptr_type self() {
            return std::enable_shared_from_this<DerivedT>::shared_from_this();
        }
    };

    template <typename... T>
    struct start{
        typedef activities::collector<T...> collector_type;
        typedef activities::accessor<T...> accessor_type;
        typedef std::shared_ptr<collector_type> collector_ptr;
        typedef boost::signals2::signal<void ()> signal_type;
        
        signal_type   _signal;
        collector_ptr _collector;
        accessor_type _accessor;
        
        template <typename ContextT>
        start(ContextT& ctx, const std::string& name): _collector(activities::collect<T...>(ctx, name)), _accessor(_collector){}
        
        collector_ptr collector() const { return _collector; }
        const accessor_type& accessor() const { return _accessor; }
        accessor_type& accessor() { return _accessor; }
        
        void operator()(){
            success();
        }
        
        template <typename CombinatorT>
        void done(CombinatorT cmb){
            boost::function<void ()> fnc([cmb](){
                cmb->operator()();
            });
            _signal.connect(fnc);
        }
        
        void success(){                
            _signal();
        }
    };
    template <typename NextT, typename... T>
    struct combinator<NextT, start<T...>>{
        typedef std::shared_ptr<NextT> next_type;
        next_type  _next;

        combinator(next_type& next): _next(next){}
        void operator()(){
            propagate();
        }
        void propagate(){
            (*_next)();
        }
    };
    
    template <typename... T>
    struct subtask<start<T...>>{
        typedef start<T...> activity_type;
        typedef subtask<start<T...>> self_type;
        
        template <typename U, typename... DependenciesU>
        friend struct subtask;
        
        subtask(const self_type& other): _activity(other._activity){}
                
        std::shared_ptr<activity_type> activity() {
            return _activity;
        }
        
        /**
         * execute task next after the current one
         * \param next the next subtask
         */
        template <typename V, typename... DependenciesV>
        self_type& done(subtask<V, DependenciesV...>& next){
            activity()->done(next._combinator);
            return *this;
        }
        
        /**
         * Arguments for the constructor of the Activity
         */
        template <typename... U>
        static self_type with(U&&... u){
            return self_type(0, u...);
        }
        
        /**
         * calls the `operator()()` of the activity and starts executing the graph
         */
        void operator()(){
            _activity->operator()();
        }
        
        /**
         * returns the shared pointer to the actiivity
         */
        std::shared_ptr<activity_type> operator->(){
            return _activity;
        }
        
        protected:
            template <typename... U>
            subtask(int, U&&... u){
                _activity = std::make_shared<activity_type>(u...);
            }
            
            std::shared_ptr<activity_type> _activity;
    };
    
    namespace detail{
        
        template <typename T>
        struct after;
        
        template <typename ActivityT, typename... DependenciesT>
        struct after<subtask<ActivityT, DependenciesT...>>{
            subtask<ActivityT, DependenciesT...>& _before;
            
            after(subtask<ActivityT, DependenciesT...>& before): _before(before){}
            
            template <typename OtherActivityT, typename... OtherDependenciesT>
            void attach(subtask<OtherActivityT, OtherDependenciesT...>& sub){
                sub.after(_before);
            }
        };
        
    }
    
    template <typename HeadT, typename... TailT>
    struct after: detail::after<HeadT>, after<TailT...>{
        after(HeadT& head, TailT&... tail): detail::after<HeadT>(head), after<TailT...>(tail...){}
        
        template <typename ActivityT, typename... Args>
        subtask<ActivityT, typename HeadT::activity_type, typename TailT::activity_type...> perform(Args&&... args){
            subtask<ActivityT, typename HeadT::activity_type, typename TailT::activity_type...> sub = subtask<ActivityT, typename HeadT::activity_type, typename TailT::activity_type...>::with(args...);
            attach(sub);
            return sub;
        }
        
        template <typename ActivityT, typename... DependenciesT>
        void attach(subtask<ActivityT, DependenciesT...>& sub){
            detail::after<HeadT>::attach(sub);
            after<TailT...>::attach(sub);
        }
        
        template <typename CallbackT, typename StartT>
        subtask<joined<CallbackT, typename StartT::collector_type>, typename HeadT::activity_type, typename TailT::activity_type...> finish(StartT& starter, CallbackT callback){
            subtask<joined<CallbackT, typename StartT::collector_type>, typename HeadT::activity_type, typename TailT::activity_type...> sub = subtask<joined<CallbackT, typename StartT::collector_type>, typename HeadT::activity_type, typename TailT::activity_type...>::with(starter.collector(), callback);
            attach(sub);
            return sub;
        }
    };
    
    template <typename HeadT>
    struct after<HeadT>: detail::after<HeadT>{
        after(HeadT& head): detail::after<HeadT>(head){}
        
        template <typename ActivityT, typename... Args>
        subtask<ActivityT, typename HeadT::activity_type> perform(Args&&... args){
            subtask<ActivityT, typename HeadT::activity_type> sub = subtask<ActivityT, typename HeadT::activity_type>::with(args...);
            attach(sub);
            return sub;
        }
        
        template <typename ActivityT, typename... DependenciesT>
        void attach(subtask<ActivityT, DependenciesT...>& sub){
            detail::after<HeadT>::attach(sub);
        }
        
        template <typename CallbackT, typename StartT>
        subtask<joined<CallbackT, typename StartT::collector_type>, typename HeadT::activity_type> finish(StartT& starter, CallbackT callback){
            subtask<joined<CallbackT, typename StartT::collector_type>, typename HeadT::activity_type> sub = subtask<joined<CallbackT, typename StartT::collector_type>, typename HeadT::activity_type>::with(starter.collector(), callback);
            attach(sub);
            return sub;
        }
    };
    
    struct after_none{
        template <typename ActivityT, typename... Args>
        subtask<ActivityT> perform(Args&&... args){
            return subtask<ActivityT>::with(args...);
        }
    };
}

/**
 * shorthand for udho::activities::collect
 * \ingroup data
 */
template <typename... T, typename ContextT>
std::shared_ptr<activities::collector<T...>> collect(ContextT& ctx, const std::string& name){
    return activities::collect<T...>(ctx, name);
}

/**
 * shorthand for udho::activities::accessor
 * \see udho::activities::accessor
 * \ingroup data
 */
template <typename... T>
using accessor = activities::accessor<T...>;

/**
 * shorthand for udho::activities::activity
 * \see udho::activities::activity
 * \ingroup activities
 */
template <typename DerivedT, typename SuccessDataT, typename FailureDataT>
using activity = activities::activity<DerivedT, SuccessDataT, FailureDataT>;

/**
 * shorthand for udho::activities::require
 * \see udho::activities::require
 * \ingroup activities
 */
template <typename... DependenciesT>
using require = activities::require<DependenciesT...>;

/**
 * shorthand for udho::activities::perform
 * \see udho::activities::perform
 * \ingroup activities
 */
template <typename ActivityT>
using perform = activities::perform<ActivityT>;

/**
 * \see udho::activities::analyzer
 * \ingroup activities
 * \ingroup data
 */
template <typename ActivityT, typename AccessorT>
udho::activities::analyzer<ActivityT> analyze(AccessorT& accessor){
    return udho::activities::analyzer<ActivityT>(accessor);
}

template <typename... T>
udho::activities::after<T...> after(T&... dependencies){
    return udho::activities::after<T...>(dependencies...);
}

inline udho::activities::after_none after(){
    return udho::activities::after_none();
}

template <typename... T>
struct start: activities::subtask<activities::start<T...>>{
    typedef activities::start<T...> activity_type;
    typedef activities::subtask<activity_type> base;
    typedef typename activity_type::collector_type collector_type;
    typedef typename activity_type::accessor_type accessor_type;
    
    template <typename ContextT>
    start(ContextT ctx, const std::string& name = "activity"): base(0, ctx, name){}
    
    auto collector() { return base::_activity->collector(); }
    auto data() const { return base::_activity->accessor(); }
    auto data() { return base::_activity->accessor(); }
    
    template <typename ActivityT>
    auto success() const {
        return udho::accessor<ActivityT>(data()).template success<ActivityT>();
    }
    template <typename ActivityT>
    auto failure() const {
        return udho::accessor<ActivityT>(data()).template failure<ActivityT>();
    }
};

namespace activities{
    namespace detail{
        
        template <typename... T>
        struct after<udho::start<T...>>{
            udho::start<T...>& _before;
            
            after(udho::start<T...>& before): _before(before){}
            
            template <typename OtherActivityT, typename... OtherDependenciesT>
            void attach(subtask<OtherActivityT, OtherDependenciesT...>& sub){
                sub.after(_before);
            }
        };
        
    }
}

}

#endif // ACTIVITIES_H
