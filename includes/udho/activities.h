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

namespace udho{
namespace activities{
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
        const V& get() const{
            return _shadow.template get<V>(key());
        }
        template <typename V>
        V& at(){
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
    
    template <typename... T>
    struct accessor: fixed_key_accessor<udho::cache::shadow<std::string, typename T::result_type...>>{
        typedef fixed_key_accessor<udho::cache::shadow<std::string, typename T::result_type...>> base_type;
        typedef udho::cache::shadow<std::string, typename T::result_type...> shadow_type;
        
        shadow_type _shadow;
        
        template <typename... U>
        accessor(std::shared_ptr<collector<U...>> collector): base_type(_shadow, collector->name()), _shadow(collector->shadow()){}
        std::string name() const{ return base_type::key(); }
        shadow_type& shadow() { return _shadow; }
        const shadow_type& shadow() const { return _shadow; }
        
        template <typename V>
        bool exists() const{
            return base_type::template exists<typename V::result_type>();
        }
        template <typename V>
        const typename V::result_type& get() const{
            return base_type::template get<typename V::result_type>();
        }
        template <typename V>
        bool failed() const{
            if(base_type::template exists<typename V::result_type>()){
                typename V::result_type res = base_type::template get<typename V::result_type>();
                return res.failed();
            }
            return true;
        }
        template <typename V>
        typename V::result_type::success_type success() const{
            if(base_type::template exists<typename V::result_type>()){
                typename V::result_type res = base_type::template get<typename V::result_type>();
                return res.success_data();
            }
            return typename V::result_type::success_type();
        }
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
    };
    
    template <typename U, typename... T>
    collector<T...>& operator<<(collector<T...>& h, const U& data){
        auto& shadow = h.shadow();
        shadow.template set<U>(h.name(), data);
        return h;
    }

    template <typename U, typename... T>
    const collector<T...>& operator>>(const collector<T...>& h, U& data){
        const auto& shadow = h.shadow();
        data = shadow.template get<U>(h.name());
        return h;
    }
    
    template <typename U, typename... T>
    accessor<T...>& operator<<(accessor<T...>& h, const U& data){
        auto& shadow = h.shadow();
        shadow.template set<U>(h.name(), data);
        return h;
    }

    template <typename U, typename... T>
    const accessor<T...>& operator>>(const accessor<T...>& h, U& data){
        auto& shadow = h.shadow();
        data = shadow.template get<U>(h.name());
        return h;
    }
    
    template <typename SuccessT, typename FailureT>
    struct result_data{
        typedef SuccessT success_type;
        typedef FailureT failure_type;
        typedef result_data<SuccessT, FailureT> self_type;
        typedef self_type result_type;
        
        bool _ready;
        bool _success;
        success_type _sdata;
        failure_type _fdata;
        
        bool ready() const{
            return _ready;
        }
        bool failed() const{
            return !_success;
        }
        const success_type& success_data() const{
            return _sdata;
        }
        const failure_type& failure_data() const{
            return _fdata;
        }
        protected:
            void success(const success_type& data){
                _sdata   = data;
                _success = true;
                _ready = true;
            }
            void failure(const failure_type& data){
                _fdata   = data;
                _success = false;
                _ready = true;
            }
    };
    
    /**
     * \begin
     * struct SomeActivitySuccessData{
     *      int status;
     *      std::string result;
     * };
     * 
     * struct SomeActivityFailureData{
     *      int code;
     *      std::string reason;
     * };
     * 
     * template <typename CollectorT>
     * struct SomeActivity: udho::result<SomeActivitySuccessData, SomeActivityFailureData>{
     *      typedef udho::result<SomeActivitySuccessData, SomeActivityFailureata> base;
     * 
     *      SomeActivity(CollectorT& c): base(c){};
     *      
     *      void operator()(){
     *          SomeActivitySuccessData data;
     *          success(data);
     *      }
     * };
     * \end
     */
    template <typename SuccessT, typename FailureT>
    struct result: result_data<SuccessT, FailureT>{
        typedef result_data<SuccessT, FailureT> data_type;
        typedef accessor<data_type> accessor_type;
        typedef typename data_type::success_type success_type;
        typedef typename data_type::failure_type failure_type;
        typedef boost::signals2::signal<void (const data_type&)> signal_type;
        
        accessor_type _shadow;
        signal_type   _signal;
        
        template <typename StoreT>
        result(StoreT& store): _shadow(store){}
        
        template <typename CombinatorT>
        void done(CombinatorT cmb){
            boost::function<void (const data_type&)> fnc([cmb](const data_type& data){
                cmb->operator()(data);
            });
            _signal.connect(fnc);
        }
        protected:
            void success(const success_type& data){
                data_type::success(data);
                completed();
            }
            void failure(const failure_type& data){
                data_type::failure(data);
                completed();
            }
        private:
            void completed(){
                data_type self = static_cast<const data_type&>(*this);
                _shadow << self;
                _signal(self);
            }
    };

    template <typename DependencyT>
    struct junction{
        void operator()(const DependencyT&){}
    };
    
    /**
     * A combinator combines multiple activities and proceeds towards the next activity
     */
    template <typename NextT, typename... DependenciesT>
    struct combinator: junction<typename DependenciesT::result_type>...{
        typedef std::shared_ptr<NextT> next_type;
        typedef boost::signals2::signal<void (NextT&)> signal_type;
        
        next_type  _next;
        std::atomic<std::size_t> _counter;
        std::mutex  _mutex;
        signal_type _signal;
        
        combinator(next_type& next): _next(next), _counter(sizeof...(DependenciesT)){}
        template <typename U>
        void operator()(const U& u){
            junction<U>::operator()(u);
            _counter--;
            if(!_counter){
                _mutex.lock();
                if(!_signal.empty()){
                    _signal(*_next);
                }
                _signal.disconnect_all_slots();
                (*_next)();
                _mutex.unlock();
            }
        }
        
        template <typename PreparatorT>
        void prepare(PreparatorT prep){
            boost::function<void (NextT&)> fnc(prep);
            _signal.connect(prep);
        }
    };
    
    
    /**
     *  A `subtask` is an instantiation of an `activity`. The subtask reuses an activity to model different use cases by attaching dependencies.
     * 
     * \tparam ActivityT The  activity 
     * \tparam DependenciesT The activities that has to be performed before performing ActivityT
     */
    template <typename ActivityT, typename... DependenciesT>
    struct subtask{
        typedef ActivityT activity_type;
        typedef combinator<ActivityT, DependenciesT...> combinator_type;
        typedef subtask<ActivityT, DependenciesT...> self_type;
        
        template <typename U, typename... DependenciesU>
        friend class subtask;
        
        subtask(const self_type& other): _activity(other._activity), _combinator(other._combinator){}
                
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
     *  Spetialization for the root subtask in the task graph
     * 
     * \tparam ActivityT The  activity 
     */
    template <typename ActivityT>
    struct subtask<ActivityT>{
        typedef ActivityT activity_type;
        typedef subtask<ActivityT> self_type;
        
        template <typename U, typename... DependenciesU>
        friend class subtask;
        
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
    
    template <typename DerivedT, typename... T>
    struct aggregated: std::enable_shared_from_this<DerivedT>{
        typedef collector<T...> collector_type;
        typedef accessor<T...> accessor_type;
        
        template <typename ContextT>
        aggregated(ContextT ctx, const std::string& name): _collector(std::make_shared<collector_type>(ctx.aux().config(), name)), _accessor(_collector){}
        
        std::shared_ptr<collector_type> data() { return _collector; }
        accessor_type& access() { return _accessor; }
        std::shared_ptr<DerivedT> self() { return std::enable_shared_from_this<DerivedT>::shared_from_this(); }
        
        template <typename V>
        bool exists() const{
            return _accessor.template exists<typename V::result_type>();
        }
        template <typename V>
        const typename V::result_type& get() const{
            return _accessor.template get<typename V::result_type>();
        }
        template <typename V>
        bool failed() const{
            if(_accessor.template exists<typename V::result_type>()){
                typename V::result_type res = _accessor.template get<typename V::result_type>();
                return res.failed();
            }
            return true;
        }
        template <typename V>
        typename V::result_type::success_type success() const{
            if(_accessor.template exists<typename V::result_type>()){
                typename V::result_type res = _accessor.template get<typename V::result_type>();
                return res.success_data();
            }
            return typename V::result_type::success_type();
        }
        template <typename V>
        typename V::result_type::failure_type failure() const{
            if(_accessor.template exists<typename V::result_type>()){
                typename V::result_type res = _accessor.template get<typename V::result_type>();
                return res.failure_data();
            }
            return typename V::result_type::failure_type();
        }
        private:
            std::shared_ptr<collector_type> _collector;
            accessor_type _accessor;
    };
    
    template <typename... T, typename ContextT>
    std::shared_ptr<collector<T...>> collect(ContextT& ctx, const std::string& name){
        return std::make_shared<collector<T...>>(ctx.aux().config(), name);
    }
    
    template <typename CollectorT, typename CallbackT>
    struct joined{
        typedef CollectorT collector_type;
        typedef typename collector_type::accessor_type accessor_type;
        typedef CallbackT callback_type;
        typedef joined<CollectorT, CallbackT> self_type;
        
        joined(std::shared_ptr<collector_type> collector, CallbackT callback): _collector(collector), _callback(callback){}
        void operator()(){
            accessor_type accessor(_collector);
            _callback(accessor);
        }
        private:
            std::shared_ptr<collector_type> _collector;
            callback_type _callback;
    };
    
//     template <typename... DependenciesT, typename CollectorT, typename CallbackT>
//     task<joined<CollectorT, CallbackT>, DependenciesT...> exec(std::shared_ptr<CollectorT> collector, CallbackT callback){
//         return task<joined<CollectorT, CallbackT>, DependenciesT...>::with(collector, callback);
//     }
    
    namespace detail{
        template <typename CollectorT, typename... DependenciesT>
        struct final_intermediate{
            std::shared_ptr<CollectorT> _collector;
            
            final_intermediate(std::shared_ptr<CollectorT> collector): _collector(collector){}
            
            template <typename CallbackT>
            subtask<joined<CollectorT, CallbackT>, DependenciesT...> exec(CallbackT callback){
                return subtask<joined<CollectorT, CallbackT>, DependenciesT...>::with(_collector, callback);
            }
        };
    }
    
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
     */
    template <typename DerivedT, typename SuccessDataT, typename FailureDataT>
    struct activity: std::enable_shared_from_this<DerivedT>, udho::activities::result<SuccessDataT, FailureDataT>{
        typedef std::shared_ptr<DerivedT> derived_ptr_type;
        
        template <typename StoreT>
        activity(StoreT& store): udho::activities::result<SuccessDataT, FailureDataT>(store){}
        
        derived_ptr_type self() {
            return std::enable_shared_from_this<DerivedT>::shared_from_this();
        }
    };
}
}

#endif // ACTIVITIES_H
