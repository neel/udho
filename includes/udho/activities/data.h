/*
 * Copyright (c) 2020, Neel Basu <neel.basu.z@gmail.com>
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *  * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  * Neither the name of the <organization> nor the
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

#ifndef UDHO_ACTIVITIES_DATA_H
#define UDHO_ACTIVITIES_DATA_H

#include <string>
#include <memory>
#include <udho/cache.h>

namespace udho{
/**
 * \ingroup activities
 */
namespace activities{

/**
 * \defgroup data data
 * data collected by activities
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

namespace detail{
    
template <typename ActivityT, typename ResultT>
struct labeled{
    typedef ActivityT activity_type;
    typedef ResultT result_type;
    typedef labeled<ActivityT, ResultT> self_type;
    
    labeled(){}
    labeled(const result_type& res): _result(res){}
    self_type& operator=(const result_type& res) { _result = res; return *this; }
    result_type get() const { return _result;}
    operator result_type() const { return get(); }
    
    private:
        result_type _result;
};

template <typename T>
struct is_labeled{
    static constexpr bool value = false;
};

template <typename ActivityT, typename ResultT>
struct is_labeled<labeled<ActivityT, ResultT>>{
    static constexpr bool value = true;
};
    
}

/**
 * dataset
 * \ingroup data
 */
template <typename... T>
struct dataset: fixed_key_accessor<udho::cache::shadow<std::string, detail::labeled<T, typename T::result_type>...>>{
    typedef fixed_key_accessor<udho::cache::shadow<std::string, detail::labeled<T, typename T::result_type>...>> base_type;
    typedef udho::cache::store<udho::cache::storage::memory, std::string, detail::labeled<T, typename T::result_type>...> store_type;
    typedef typename store_type::shadow_type shadow_type;
    typedef accessor<T...> accessor_type;
    
    store_type   _store;
    shadow_type  _shadow;
    std::string  _name;
    
    dataset(const udho::configuration_type& config, const std::string& name): base_type(_shadow, name), _store(config), _shadow(_store), _name(name){}
    std::string name() const{ return _name; }
    shadow_type& shadow() { return _shadow; }
    const shadow_type& shadow() const { return _shadow; }
};

template <typename ContextT, typename DatasetT>
struct collector;

/**
 * Collects data associated with all activities involved in the subtask graph
 * \ingroup data
 */
template <typename ContextT, typename... T>
struct collector<ContextT, dataset<T...>>: dataset<T...>, std::enable_shared_from_this<collector<ContextT, dataset<T...>>>{
    typedef dataset<T...> base_type;
    typedef ContextT context_type;
    
    context_type _context;
    
    collector(context_type ctx, const std::string& name): base_type(ctx.aux().config(), name), _context(ctx) {}
    context_type& context() { return _context; }
    const context_type& context() const { return _context; }
};

/**
 * Access a subset of data from the collector
 * \ingroup data
 */
template <typename... T>
struct accessor: fixed_key_accessor<udho::cache::shadow<std::string, typename std::conditional<detail::is_labeled<T>::value, T, detail::labeled<T, typename T::result_type>>::type...>>{
    typedef fixed_key_accessor<udho::cache::shadow<std::string, typename std::conditional<detail::is_labeled<T>::value, T, detail::labeled<T, typename T::result_type>>::type...>> base_type;
    typedef udho::cache::shadow<std::string, typename std::conditional<detail::is_labeled<T>::value, T, detail::labeled<T, typename T::result_type>>::type...> shadow_type;
    
    shadow_type _shadow;
    
    template <typename ContextT, typename... U>
    accessor(std::shared_ptr<collector<ContextT, dataset<U...>>> collector): base_type(_shadow, collector->name()), _shadow(collector->shadow()){}
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
        return base_type::template exists<detail::labeled<V, typename V::result_type>>();
    }
    /**
     * get data associated with activity V
     * 
     * \tparam V activity type
     */
    template <typename V>
    const typename V::result_type& get() const{
        return base_type::template get<detail::labeled<V, typename V::result_type>>();
    }
    /**
     * Check whether activity V has completed.
     * \tparam V activity type
     */
    template <typename V>
    bool completed() const{
        if(base_type::template exists<detail::labeled<V, typename V::result_type>>()){
            typename V::result_type res = base_type::template get<detail::labeled<V, typename V::result_type>>();
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
        if(base_type::template exists<detail::labeled<V, typename V::result_type>>()){
            typename V::result_type res = base_type::template get<detail::labeled<V, typename V::result_type>>();
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
        if(base_type::template exists<detail::labeled<V, typename V::result_type>>()){
            typename V::result_type res = base_type::template get<detail::labeled<V, typename V::result_type>>();
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
        if(base_type::template exists<detail::labeled<V, typename V::result_type>>()){
            typename V::result_type res = base_type::template get<detail::labeled<V, typename V::result_type>>();
            return res.okay();
        }
        return false;
    }
    /**
     * get success data for activity V
     * \tparam V activity type
     */
    template <typename V>
    typename V::result_type::success_type success() const{
        if(base_type::template exists<detail::labeled<V, typename V::result_type>>()){
            typename V::result_type res = base_type::template get<detail::labeled<V, typename V::result_type>>();
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
        if(base_type::template exists<detail::labeled<V, typename V::result_type>>()){
            typename V::result_type res = base_type::template get<detail::labeled<V, typename V::result_type>>();
            return res.failure_data();
        }
        return typename V::result_type::failure_type();
    }
    template <typename V>
    void set(const typename V::result_type& value){
        base_type::template set<detail::labeled<V, typename V::result_type>>(value);
    }
    /**
     * Apply a callback on result of V
     * \tparam V activity type
     * \param f callback
     */
    template <typename V, typename F>
    void apply(F f) const{
        if(base_type::template exists<detail::labeled<V, typename V::result_type>>()){
            typename V::result_type res = base_type::template get<detail::labeled<V, typename V::result_type>>();
            res.template apply<F>(f);
        }
    }
};

/**
 * \ingroup data
 */
template <typename U, typename ContextT, typename... T>
collector<ContextT, dataset<T...>>& operator<<(collector<ContextT, dataset<T...>>& h, const U& data){
    auto& shadow = h.shadow();
    shadow.template set<U>(h.name(), data);
    return h;
}

/**
 * \ingroup data
 */
template <typename U, typename ContextT, typename... T>
const collector<ContextT, dataset<T...>>& operator>>(const collector<ContextT, dataset<T...>>& h, U& data){
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

/**
 * \ingroup data
 */
template <typename... T, typename ContextT>
std::shared_ptr<collector<ContextT, dataset<T...>>> collect(ContextT& ctx, const std::string& name){
    typedef collector<ContextT, dataset<T...>> collector_type;
    return std::make_shared<collector_type>(ctx, name);
}
    
}
}
#endif // UDHO_ACTIVITIES_DATA_H
