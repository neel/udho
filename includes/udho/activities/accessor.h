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
#include <type_traits>
#include <udho/activities/detail.h>
#include <udho/activities/fwd.h>
#include <udho/activities/collector.h>
#include <udho/hazo/node/proxy.h>
#include <boost/shared_ptr.hpp>

namespace udho{
namespace activities{

/**
 * @brief Checks whether the AccessorT is superset of SubAccessorT.
 * AccessorT (e.g. accessor<X...>) is superset of SubAccessorT (e.g. accessor<S...>) if S... is a subset of X... which means all s in S... must exist in X...
 * 
 * @tparam AccessorT 
 * @tparam SubAccessorT 
 * @ingroup activities
 */
template <typename AccessorT, typename SubAccessorT>
struct is_superset_of;

/**
 * @brief Checks whether the SubAccessorT is subset of AccessorT.
 * 
 * @tparam SubAccessorT 
 * @tparam AccessorT 
 * @ingroup activities
 * @see is_superset_of
 */
template <typename SubAccessorT, typename AccessorT>
using is_subset_of = is_superset_of<AccessorT, SubAccessorT>;

/**
 * @brief To mark a class X as Accessible (e.g. an accessor can be retrieved from and instance of X) specialize accessor_of<X>
 * The specialization accessor_of<X> must provide a typedef named `type` which returns the type of the appropriate accessot.
 * The specialization must also provide a static apply(X& x) method that returns an accessor of the above mentioned type from x
 * @note If X is accessible then std::shared_ptr<X> is also accessible. No need to specialize std::shared_ptr or boost::shared_ptr
 * @tparam AccessibleT 
 * @ingroup activities
 */
template <typename AccessibleT>
struct accessor_of{
    using type = void;
    type apply(const AccessibleT&) {}
};

/**
 * @brief Checks whether the given type Xis accessible (e.g. an accessor can be retrieved from and instance of X)
 * 
 * @tparam AccessibleT 
 * @ingroup activities
 * @see accessor_of
 */
template <typename AccessibleT>
using is_accessible = std::integral_constant<bool, !std::is_void<typename accessor_of<AccessibleT>::type>::value>;

/**
 * @brief Retrieve the accessor from an Accessible
 * @see accessor_of
 * @tparam AccessibleT 
 * @param accessible 
 * @return accessor_of<AccessibleT>::type 
 * @ingroup activities
 */
template <typename AccessibleT>
typename accessor_of<AccessibleT>::type accessor_from(const AccessibleT& accessible){
    return accessor_of<AccessibleT>::apply(accessible);
}

#ifndef __DOXYGEN__

template <typename... X, typename Head, typename... Tail>
struct is_superset_of<accessor<X...>, accessor<Head, Tail...>>: std::integral_constant<bool, is_superset_of<accessor<X...>, accessor<Head>>::value || is_superset_of<accessor<X...>, accessor<Tail...>>::value> {};

template <typename... X, typename Y>
struct is_superset_of<accessor<X...>, accessor<Y>>: std::integral_constant<bool, accessor<X...>::types::template exists<Y>::value> {};

template <typename AccessibleT>
struct accessor_of<std::shared_ptr<AccessibleT>>{
    using type = typename accessor_of<AccessibleT>::type;
    static type apply(std::shared_ptr<AccessibleT> ptr){ return accessor_of<AccessibleT>::apply(*ptr); }
    static type apply(std::shared_ptr<AccessibleT>& ptr){ return accessor_of<AccessibleT>::apply(*ptr); }
};

template <typename AccessibleT>
struct accessor_of<boost::shared_ptr<AccessibleT>>{
    using type = typename accessor_of<AccessibleT>::type;
    static type apply(boost::shared_ptr<AccessibleT> ptr){ return accessor_of<AccessibleT>::apply(*ptr); }
    static type apply(boost::shared_ptr<AccessibleT>& ptr){ return accessor_of<AccessibleT>::apply(*ptr); }
};

template <typename ContextT, typename... T>
struct accessor_of<collector<ContextT, T...>>{
    using type = accessor<T...>;
    static type apply(collector<ContextT, T...>& collector){ return type(collector); }
};

template <typename... T>
struct accessor_of<accessor<T...>>{
    using type = accessor<T...>;
    static type apply(accessor<T...>& accessor){ return accessor; }
};

#endif 

/**
 * @brief Access a subset of data from the collector. 
 * Given a collector collecting result data of different activities, multiple accessors can be created to access
 * the full or a subset of the data collected by the collector. For example, given a collector of type 
 * @ref udho::activities::collector `collector<ContextT, A1, A2, A3>` there can be an @ref udho::activities::accessor `accessor<A1, A2>`
 * that provides a read write access to the success and failre results of A1 and A2 activities only. While 
 * instantiating an activity `A1` the collector is passed, because the @ref udho::activities::activity "activity<A1, ...>"
 * base class constructs an `accessor<A1>` to store the success or failure result of the acitivity. When `A1` 
 * calls the @ref udho::activities::activity::success "activity::success(s)" the value s is stored into the 
 * collector via that internal accessor. Thats why it is essential for the `A1` constructor to take a collector
 * (or an accessor, or any other object from which an accessor can be extracted) as argument and pass that to 
 * the activity base class. 
 * @ingroup activities
 * @tparam Activities... A set of Activities (which might be labeled with its result type)
 */
template <typename... Activities>
struct accessor: private udho::hazo::proxy<typename std::conditional<detail::is_labeled<Activities>::value, Activities, detail::labeled<Activities, typename Activities::result_type>>::type...>{
#ifndef __DOXYGEN__
    typedef udho::hazo::proxy<typename std::conditional<detail::is_labeled<Activities>::value, Activities, detail::labeled<Activities, typename Activities::result_type>>::type...> base_type;
#endif 
    template <typename... X>
    friend class accessor;

    /**
     * @brief type assistance of an accessor
     * 
     */
    struct types{
        /**
         * @brief Checks whether an other accessor is compatiable with the current accessor
         * 
         * @tparam OtherAccessorT 
         */
        template <typename OtherAccessorT>
        using compatiable_with = typename is_subset_of<accessor<Activities...>, OtherAccessorT>::type;
        template<typename X>
        using labeled = typename std::conditional<detail::is_labeled<X>::value, X, detail::labeled<X, typename X::result_type>>::type;
        template <typename X>
        using exists = typename base_type::types::template exists<labeled<X>>;
    };

    /**
     * @brief Construct an accessor using a shared pointer to a compatiable collector
     * @tparam ContextT 
     * @tparam U... 
     * @param collector 
     */
    template <typename ContextT, typename... U, std::enable_if_t<types::template compatiable_with<accessor<U...>>::value, bool> = true >
    accessor(std::shared_ptr<collector<ContextT, U...>> collector): accessor(*collector){}

    /**
     * @brief Construct an accessor using a shared pointer to a compatiable collector
     * @tparam ContextT 
     * @tparam U... 
     * @param collector 
     */
    template <typename ContextT, typename... U, std::enable_if_t<types::template compatiable_with<accessor<U...>>::value, bool> = true >
    accessor(collector<ContextT, U...>& collector): base_type(collector.node()){}
    /**
     * @brief Construct an accessor using another compatiable accessor
     * @tparam U...
     * @param accessor 
     */
    template <typename... U, std::enable_if_t<types::template compatiable_with<accessor<U...>>::value, bool> = true >
    accessor(accessor<U...> accessor): base_type(accessor.proxy()) {}
    
    /// @name state
    /// @{
    /**
     * @brief Checks Whether there exists any data for activity V and that data is initialized
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
     * @brief Check whether activity V has completed.
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
     * @brief Check whether activity V has been canceled.
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
     * @brief Check whether activity V has failed (only the failure data of V is valid).
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
     * @brief Check whether activity V is okay.
     * @tparam V activity type
     */
    template <typename V>
    bool okay() const{
        if(exists<V>()){
            return get<V>().okay();
        }
        return false;
    }
    /// @}

    /// @name data
    /// @{
    /**
     * @brief get data associated with activity V
     * @tparam V activity type
     */
    template <typename V>
    const typename V::result_type& get() const{
        using type = typename types::template labeled<V>;
        return base_type::template data<type>().get();
    }
    /**
     * @brief get success data for activity V
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
     * @brief get failure data for activity V
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
     * @brief Apply a callback on result of V
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
    /// @}

#ifndef __DOXYGEN__
    private:
        base_type& proxy() { return *this; }
        const base_type& proxy() const { return *this; }
#endif 
};

/**
 * @ingroup activities
 */
template <typename U, typename... T>
accessor<T...>& operator<<(accessor<T...>& h, const U& data){
    h.template set<U>(data);
    return h;
}

/**
 * @ingroup activities
 */
template <typename U, typename... T>
const accessor<T...>& operator>>(const accessor<T...>& h, U& data){
    data = h.template get<U>();
    return h;
}

}
}
#endif // UDHO_ACTIVITIES_ACCESSOR_H
