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

#ifndef UDHO_ACTIVITIES_ACCESSOR_H
#define UDHO_ACTIVITIES_ACCESSOR_H

#include <string>
#include <memory>
#include <udho/cache.h>
#include <udho/activities/detail.h>
#include <udho/activities/fwd.h>
#include <udho/activities/dataset.h>
#include <udho/activities/collector.h>

namespace udho{
/**
 * @ingroup activities
 */
namespace activities{

/**
 * Access a subset of data from the collector
 * @ingroup data
 */
template <typename... T>
struct accessor: detail::fixed_key_accessor<udho::cache::shadow<std::string, typename std::conditional<detail::is_labeled<T>::value, T, detail::labeled<T, typename T::result_type>>::type...>>{
    typedef detail::fixed_key_accessor<udho::cache::shadow<std::string, typename std::conditional<detail::is_labeled<T>::value, T, detail::labeled<T, typename T::result_type>>::type...>> base_type;
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
     * @tparam V Activity Type
     */
    template <typename V>
    bool exists() const{
        return base_type::template exists<detail::labeled<V, typename V::result_type>>();
    }
    /**
     * get data associated with activity V
     * 
     * @tparam V activity type
     */
    template <typename V>
    typename V::result_type get() const{
        return base_type::template get<detail::labeled<V, typename V::result_type>>();
    }
    /**
     * Check whether activity V has completed.
     * @tparam V activity type
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
     * @tparam V activity type
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
     * @tparam V activity type
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
     * @tparam V activity type
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
     * @tparam V activity type
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
     * @tparam V activity type
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
     * @tparam V activity type
     * @param f callback
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
 * @ingroup data
 */
template <typename U, typename... T>
accessor<T...>& operator<<(accessor<T...>& h, const U& data){
    auto& shadow = h.shadow();
    shadow.template set<U>(h.name(), data);
    return h;
}

/**
 * @ingroup data
 */
template <typename U, typename... T>
const accessor<T...>& operator>>(const accessor<T...>& h, U& data){
    auto& shadow = h.shadow();
    data = shadow.template get<U>(h.name());
    return h;
}

template <typename DatasetT>
struct accessor_of;

template <typename ... T>
struct accessor_of<dataset<T...>>{
    using type = accessor<T...>;
};

template <typename ContextT, typename DatasetT>
struct accessor_of<collector<ContextT, DatasetT>>{
    using type = typename accessor_of<DatasetT>::type;
};

}
}
#endif // UDHO_ACTIVITIES_ACCESSOR_H
