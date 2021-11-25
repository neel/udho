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
#include <udho/activities/detail.h>
#include <udho/activities/fwd.h>
#include <udho/activities/collector.h>
#include <udho/hazo/node/proxy.h>

namespace udho{
/**
 * @ingroup activities
 */
namespace activities{

#ifndef __DOXYGEN__

namespace detail{
    /**
     * @brief Checks whether the AccessorT is is_superset_of SubAccessorT.
     * AccessorT (e.g. accessor<X...>) is is_superset_of SubAccessorT (e.g. accessor<S...>) if S... is a subset of X... which means all s in S... must exist in X...
     * @tparam AccessorT 
     * @tparam SubAccessorT 
     */
    template <typename AccessorT, typename SubAccessorT>
    struct is_superset_of;

    template <typename... X, typename Head, typename... Tail>
    struct is_superset_of<accessor<X...>, accessor<Head, Tail...>>{
        using type = std::integral_constant<bool, is_superset_of<accessor<X...>, accessor<Head>>::value || is_superset_of<accessor<X...>, accessor<Tail...>>::value>;
        enum { value = type::value };
    };

    template <typename... X, typename Y>
    struct is_superset_of<accessor<X...>, accessor<Y>>{
        using type = std::integral_constant<bool, accessor<X...>::types::template exists<Y>::value>;
        enum { value = type::value };
    };
    template <typename SubAccessorT, typename AccessorT>
    using is_subset_of = is_superset_of<AccessorT, SubAccessorT>;
}

#endif 

/**
 * @brief Access a subset of data from the collector
 * 
 * @ingroup data
 * @tparam Activities... A set of Activities (which might be labeled with its result type)
 */
template <typename... Activities>
struct accessor: private udho::hazo::proxy<typename std::conditional<detail::is_labeled<Activities>::value, Activities, detail::labeled<Activities, typename Activities::result_type>>::type...>{
    typedef udho::hazo::proxy<typename std::conditional<detail::is_labeled<Activities>::value, Activities, detail::labeled<Activities, typename Activities::result_type>>::type...> base_type;
   
    template <typename... X>
    friend class accessor;

    struct types{
        template <typename OtherAccessorT>
        using compatiable_with = typename detail::is_subset_of<accessor<Activities...>, OtherAccessorT>::type;
        template<typename X>
        using labeled = typename std::conditional<detail::is_labeled<X>::value, X, detail::labeled<X, typename X::result_type>>::type;
        template <typename X>
        using exists = typename base_type::types::template exists<labeled<X>>;
    };

    /**
     * @brief Construct an accessor using a shared pointer to a compatiable collector
     * 
     * @tparam ContextT 
     * @tparam U... 
     * @param collector 
     */
    template <typename ContextT, typename... U, std::enable_if_t<types::template compatiable_with<accessor<U...>>::value, bool> = true >
    accessor(std::shared_ptr<collector<ContextT, U...>> collector): base_type(collector->node()){}
    /**
     * @brief Construct an accessor using another compatiable accessor
     * 
     * @tparam U...
     * @param accessor 
     */
    template <typename... U, std::enable_if_t<types::template compatiable_with<accessor<U...>>::value, bool> = true >
    accessor(accessor<U...> accessor): base_type(accessor.proxy()) {}
    
    /**
     * Checks Whether there exists any data for activity V and that data is initialized
     * @tparam V Activity Type
     */
    template <typename V>
    bool exists() const{
        using type = typename types::template labeled<V>;
        if(types::template exists<type>::value){
            const typename types::template labeled<V>& labeled_data = base_type::template data<type>();
            return labeled_data.initialized();
        }
        return false;
    }
    /**
     * get data associated with activity V
     * 
     * @tparam V activity type
     */
    template <typename V>
    const typename V::result_type& get() const{
        using type = typename types::template labeled<V>;
        return base_type::template data<type>().get();
    }
    /**
     * Check whether activity V has completed.
     * @tparam V activity type
     */
    template <typename V>
    bool completed() const{
        if(exists<V>()){
            return get<V>().completed();
        }
        return false;
    }
    /**
     * Check whether activity V has been canceled.
     * @tparam V activity type
     */
    template <typename V>
    bool canceled() const{
        if(exists<V>()){
            return get<V>().canceled();
        }
        return false;
    }
    /**
     * Check whether activity V has failed (only the failure data of V is valid).
     * @tparam V activity type
     */
    template <typename V>
    bool failed() const{
        if(exists<V>()){
            return get<V>().failed();
        }
        return true;
    }
    /**
     * Check whether activity V is okay.
     * @tparam V activity type
     */
    template <typename V>
    bool okay() const{
        if(exists<V>()){
            return get<V>().okay();
        }
        return false;
    }
    /**
     * get success data for activity V
     * @tparam V activity type
     */
    template <typename V>
    typename V::result_type::success_type success() const{
        if(exists<V>()){
            return get<V>().success_data();
        }
        return typename V::result_type::success_type();
    }
    /**
     * get failure data for activity V
     * @tparam V activity type
     */
    template <typename V>
    typename V::result_type::failure_type failure() const{
        if(exists<V>()){
            return get<V>().failure_data();
        }
        return typename V::result_type::failure_type();
    }
    template <typename V>
    void set(const typename V::result_type& value){
        using type = typename std::conditional<detail::is_labeled<V>::value, V, detail::labeled<V, typename V::result_type>>::type;
        base_type::template data<type>() = value;
    }
    /**
     * Apply a callback on result of V
     * @tparam V activity type
     * @param f callback
     */
    template <typename V, typename F>
    void apply(F f) const{
        if(exists<V>()){
            typename V::result_type d = get<V>();
            d.template apply<F>(f);
        }
    }
    private:
        base_type& proxy() { return *this; }
        const base_type& proxy() const { return *this; }
};

/**
 * @ingroup data
 */
template <typename U, typename... T>
accessor<T...>& operator<<(accessor<T...>& h, const U& data){
    h.template set<U>(data);
    return h;
}

/**
 * @ingroup data
 */
template <typename U, typename... T>
const accessor<T...>& operator>>(const accessor<T...>& h, U& data){
    data = h.template get<U>();
    return h;
}

template <typename CollectorT>
struct accessor_of;

template <typename ContextT, typename... T>
struct accessor_of<collector<ContextT, T...>>{
    using type = accessor<T...>;
};

}
}
#endif // UDHO_ACTIVITIES_ACCESSOR_H
