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
        bool remove(){
            return _shadow.remove(key());
        }
        template <typename V>
        bool remove(){
            return _shadow.template remove<V>(key());
        }
    };
       
    template <typename... T>
    struct collector: fixed_key_accessor<udho::cache::shadow<std::string, T...>>, std::enable_shared_from_this<collector<T...>>{
        typedef fixed_key_accessor<udho::cache::shadow<std::string, T...>> base_type;
        typedef udho::cache::store<udho::cache::storage::memory, std::string, T...> store_type;
        typedef typename store_type::shadow_type shadow_type;
        
        store_type  _store;
        shadow_type _shadow;
        std::string _name;
        
        collector(const udho::configuration_type& config, const std::string& name): base_type(_shadow, name), _store(config), _shadow(_store), _name(name){}
        std::string name() const{ return _name; }
        shadow_type& shadow() { return _shadow; }
        const shadow_type& shadow() const { return _shadow; }
    };
    
    template <typename... T>
    struct accessor: fixed_key_accessor<udho::cache::shadow<std::string, T...>>{
        typedef fixed_key_accessor<udho::cache::shadow<std::string, T...>> base_type;
        typedef udho::cache::shadow<std::string, T...> shadow_type;
        
        shadow_type _shadow;
        
        template <typename... U>
        accessor(std::shared_ptr<collector<U...>> collector): base_type(_shadow, collector->name()), _shadow(collector->shadow()){}
        std::string name() const{ return base_type::key(); }
        shadow_type& shadow() { return _shadow; }
        const shadow_type& shadow() const { return _shadow; }
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
    
    template <typename NextT, typename... DependenciesT>
    struct combinator: junction<typename DependenciesT::result_type>...{
        typedef std::shared_ptr<NextT> next_type;
        
        next_type  _next;
        std::atomic<std::size_t> _counter;
        std::mutex  _mutex;
        
        combinator(next_type& next): _next(next), _counter(sizeof...(DependenciesT)){}
        template <typename U>
        void operator()(U& u){
//             junction<U>::operator()(u);
            _counter--;
            if(!_counter){
                _mutex.lock();
                (*_next)();
                _mutex.unlock();
            }
        }
    };
    
    template <typename T, typename... DependenciesT>
    struct task{
        typedef T activity_type;
        typedef combinator<T, DependenciesT...> combinator_type;
        typedef task<T, DependenciesT...> self_type;
        
        template <typename U, typename... DependenciesU>
        friend class task;
        
        task(const self_type& other): _activity(other._activity), _combinator(other._combinator){}
                
        std::shared_ptr<activity_type> activity() {
            return _activity;
        }
        
        template <typename V, typename... DependenciesV>
        self_type& done(task<V, DependenciesV...>& next){
            activity()->done(next._combinator);
            return *this;
        }
        
        template <typename... U>
        static self_type with(U&&... u){
            return self_type(0, u...);
        }
        
        private:
            template <typename... U>
            task(int, U&&... u){
                _activity = std::make_shared<activity_type>(u...);
                _combinator = std::make_shared<combinator_type>(_activity);
            }
            
            std::shared_ptr<activity_type> _activity;
            std::shared_ptr<combinator_type> _combinator;
    };
    
    template <typename T>
    struct task<T>{
        typedef T activity_type;
        typedef task<T> self_type;
        
        task(const self_type& other): _activity(other._activity){}
                
        std::shared_ptr<activity_type> activity() {
            return _activity;
        }
                
        template <typename V, typename... DependenciesV>
        self_type& done(task<V, DependenciesV...>& next){
            activity()->done(next._combinator);
            return *this;
        }
        
        template <typename... U>
        static self_type with(U&&... u){
            return self_type(0, u...);
        }
        
        template <typename... U>
        void operator()(U&&... u){
            _activity->operator()(u...);
        }
        
        private:
            template <typename... U>
            task(int, U&&... u){
                _activity = std::make_shared<activity_type>(u...);
            }
            
            std::shared_ptr<activity_type> _activity;
    };
}
}

#endif // ACTIVITIES_H
