/*
 * Copyright (c) 2020, <copyright holder> <email>
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
 * THIS SOFTWARE IS PROVIDED BY <copyright holder> <email> ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <copyright holder> <email> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#ifndef UDHO_HAZO_SEQ_BASIC_H
#define UDHO_HAZO_SEQ_BASIC_H

#include <utility>
#include <type_traits>
#include <udho/hazo/node/node.h>
#include <udho/hazo/seq/fwd.h>
#include <udho/hazo/seq/tag.h>
#include <udho/hazo/seq/helpers.h>
#include <udho/hazo/detail/indices.h>
#include <udho/hazo/operations/flatten.h>

namespace udho{
namespace hazo{

#ifndef __DOXYGEN__    

template <typename Policy, typename H, typename... X>
struct basic_seq: basic_node<H, basic_seq<Policy, X...>>{
    typedef basic_node<H, basic_seq<Policy, X...>> node_type;
    
    typedef seq_proxy<Policy, H, X...> proxy;
    
    using hana_tag = udho_hazo_seq_tag<Policy, 1+sizeof...(X)>;
    
    template <typename... U>
    using exclude = typename operations::exclude<basic_seq<Policy, H, X...>, U...>::type;
    template <typename... U>
    using extend = typename operations::append<basic_seq<Policy, H, X...>, U...>::type;
    template <template <typename...> class ContainerT>
    using translate = ContainerT<H, X...>;
    template <typename T>
    using contains = typename node_type::types::template exists<T>;
    template <typename KeyT>
    using has = typename node_type::types::template has<KeyT>;
    template <template <typename> class F>
    using transform = basic_seq<Policy, F<H>, F<X>...>;
    
    using node_type::node_type;
    template <typename FunctionT>
    decltype(auto) unpack(FunctionT&& f) const{
        const_call_helper<Policy, node_type, typename build_indices<1+sizeof...(X)>::indices_type> helper(*this);
        return helper.apply(std::forward<FunctionT>(f));
    }
    template <typename FunctionT>
    decltype(auto) unpack(FunctionT&& f){
        call_helper<Policy, node_type, typename build_indices<1+sizeof...(X)>::indices_type> helper(*this);
        return helper.apply(std::forward<FunctionT>(f));
    }
};

template <typename Policy, typename H>
struct basic_seq<Policy, H>: basic_node<H, void>{
    typedef basic_node<H, void> node_type;
    
    typedef seq_proxy<Policy, H> proxy;
    
    using hana_tag = udho_hazo_seq_tag<Policy, 1>;
    
    template <typename... U>
    using exclude = typename operations::exclude<basic_seq<Policy, H>, U...>::type;
    template <typename... U>
    using extend = typename operations::append<basic_seq<Policy, H>, U...>::type;
    template <template <typename...> class ContainerT>
    using translate = ContainerT<H>;
    template <typename T>
    using contains = typename node_type::types::template exists<T>;
    template <typename KeyT>
    using has = typename node_type::types::template has<KeyT>;
    template <template <typename> class F>
    using transform = basic_seq<Policy, F<H>>;
    
    using node_type::node_type;
    template <typename FunctionT>
    decltype(auto) unpack(FunctionT&& f) const{
        const_call_helper<Policy, node_type, typename build_indices<1>::indices_type> helper(*this);
        return helper.apply(std::forward<FunctionT>(f));
    }
    template <typename FunctionT>
    decltype(auto) unpack(FunctionT&& f){
        call_helper<Policy, node_type, typename build_indices<1>::indices_type> helper(*this);
        return helper.apply(std::forward<FunctionT>(f));
    }
};

template <typename... X>
using basic_seq_d = basic_seq<by_data, X...>;
template <typename... X>
using basic_seq_v = basic_seq<by_value, X...>;

#else

/**
 * @brief Index based Sequential access to heterogenous node container that models boost::hana::Sequence Concept
 * The basic_seq provides a basic sequential container for heterogenous types using the @basic_node. So all functionalities of @ref basic_node
 * can be used on @ref basic_seq.
 * 
 * udho::hazo::seq_d and udho::hazo::seq_v models boost::hana::Sequence Concept. Hence it also models Iterable, Foldable concepts too. 
 * It can be used in cases where heterogenous containers like boost::hana::tuple is applicable. All boost::hana functions that work on
 * Iterable, Foldable and Sequence can also be applied on seq.
 *
 * Its behavour depends on the policy used to access the items on the container. The two possible policies are @ref udho::hazo::by_data
 * and @ref udho::hazo::by_value which affects the unpack method as well as the hana extensions. With @ref udho::hazo::by_data policy
 * the data inside a node is used as the content. On the other hand with @ref udho::hazo::by_value policy the value of the node is used 
 * as the content of a node. @ref udho::hazo::seq_d specializes @ref udho::hazo::basic_seq with @ref udho::hazo::by_data policy and 
 * @ref udho::hazo::seq_v specializes @ref udho::hazo::basic_seq with @ref udho::hazo::by_value policy.
 *
 * However both of these policies behave similarly if the sequence is composed with pod type or with classes that neither don't provide
 * a value_type typedef and value() method to get the value of the data inside. The difference between these two policies are visible
 * when hazo containers are used with @ref udho::hazo::element or some variant of elements eg. @ref HAZO_ELEMENT, @ref HAZO_ELEMENT_HANA.
 *
 * Following example demonstrate the basic usage of hazo sequence
 * @code {.cpp}
 * seq_d<int, std::string, double, int> vec(42, "Hello", 3.14, 84);
 * std::cout << vec.data<int>() << " " << vec.data<0>() << std::endl;
 * std::cout << vec.data<std::string>() << " " << vec.data<1>() << std::endl;
 * std::cout << vec.data<double>() << " " << vec.data<2>() << std::endl;
 * std::cout << vec.data<int, 1>() << " " << vec.data<3>() << std::endl;
 * vec.data<0>() = 24;
 * std::cout << vec.data<int>() << " " << vec.data<0>() << std::endl;
 * vec.data<int>() = 100;
 * std::cout << vec.data<int>() << " " << vec.data<0>() << std::endl;
 * vec.data<int, 1>() = 100;
 * std::cout << vec.data<int, 1>() << " " << vec.data<3>() << std::endl;
 * @endcode
 * The above prints
 * @code {.bash}
 * 42 42
 * Hello Hello
 * 3.14 3.14
 * 84 84
 * 24 24
 * 100 100
 * 100 100
 * @endcode
 * In the above example both data and value returns the same output, because only pod types are used.
 * The conveniance function @ref make_seq_d @ref make_seq_v can also be used as shown in the example below.
 * @code {.cpp}
 * make_seq_d(42, 34.5, "World")
 * make_seq_v(42, 34.5, "World")
 * @endcode
 * The values can be unpacked to a function provided.
 * @code {.cpp}
 * auto add = [](auto x, auto y, auto z) {
 *      return x + y + z;
 * };
 * auto tpl = make_seq_v(1, 2, 3);
 * std::cout << tpl.unpack(tpl) << std::endl; // prints 6
 * @endcode
 * boost hana functions can also be applied on hazo sequence
 * @code {.cpp}
 * namespace hana = boost::hana;
 * hana::unpack(tpl, add); // returns 6
 * seq_v<int, std::string, double, int> vec_v(42, "Hello", 3.14, 84);
 * hana::at(vec_v, hana::size_t<0>{}); // returns 42
 * hana::at(vec_v, hana::size_t<1>{}); // returns "Hello"
 * auto to_string = [](auto x) {
 *     std::ostringstream ss;
 *     ss << x;
 *     return ss.str();
 * };
 * auto lhs = make_seq_v(1, '2', "345", std::string{"67"});
 * auto rhs = make_seq_v("1", "2", "345", "67");
 * std::cout << (hana::transform(lhs, to_string) == rhs) << std::endl; // prints 1
 * hana::fill(make_seq_v(1, '2', 3.3, nullptr), 'x'); // returns make_seq_v('x', 'x', 'x', 'x')
 * auto negate = [](auto x) { return -x; };
 * hana::adjust(make_seq_v(1, 4, 9, 2, 3, 4), 4, negate); // returns make_seq_v(1, -4, 9, 2, 3, -4)
 * @endcode
 * 
 * @tparam Policy @ref udho::hazo::by_data or @ref udho::hazo::by_value
 * @tparam X...
 * @ingroup hazo
 * @see basic_node
 */
template <typename Policy, typename... X>
struct basic_seq{
    typedef basic_node<H, basic_seq<Policy, X...>> node_type;
    typedef seq_proxy<Policy, H, X...> proxy;
    
    /**
     * @brief returns a new sequence with the same policy but without the types U..
     * @tparam U...
     */
    template <typename... U>
    using exclude = typename operations::exclude<basic_seq<Policy, H, X...>, U...>::type;
    /**
     * @brief returns a new sequence with the same policy which is extended with the types U..
     * @tparam U...
     */
    template <typename... U>
    using extend = typename operations::append<basic_seq<Policy, H, X...>, U...>::type;
    /**
     * @brief translate to another contaner. e.g. translate a sequence to a map
     * @tparam ContainerT 
     */
    template <template <typename...> class ContainerT>
    using translate = ContainerT<H, X...>;
    /**
     * @brief checks whether the sequence contains the provided type T
     * @tparam T 
     */
    template <typename T>
    using contains = typename node_type::types::template exists<T>;
    /**
     * @brief checks whether the sequence has any element identifiable by KeyT
     * @tparam KeyT 
     */
    template <typename KeyT>
    using has = typename node_type::types::template has<KeyT>;
    /**
     * @brief transform each type in a sequence to another type by using the template F
     * @tparam F 
     */
    template <template <typename> class F>
    using transform = basic_seq<Policy, F<H>, F<X>...>;
    
    /**
     * @brief Construct a new basic sequence without any value
     * @note if no value is specified the items are constructed using their corresponding default constructors, which may lead to garbage values for the pod types
     */
    basic_seq();
    /**
     * @brief Construct a new basic sequence with one or more value(s)
     * @note number of arguments provided must be less or equal to the items in the sequence.
     * @note the arguments must be provided in the same order as in the container
     * @tparam T...
     */
    template <typename... T>
    basic_seq(const T&...);
    /**
     * @brief unpacks the items in the sequence and passes them to a function
     * @note the provided function must accept same number and type of arguments
     * @tparam FunctionT 
     * @param f the function to call
     * @return returns whatever the function f return
     */
    template <typename FunctionT>
    decltype(auto) unpack(FunctionT&& f) const;
    /**
     * @brief unpacks the items in the sequence and passes them to a function
     * @note the provided function must accept same number and type of arguments
     * @tparam FunctionT 
     * @param f the function to call
     * @return returns whatever the function f return
     */
    template <typename FunctionT>
    decltype(auto) unpack(FunctionT&& f);
};

/**
 * @brief hazo basic sequence specialized with @ref udho::hazo::by_data policy
 * @tparam X...
 * @see basic_seq
 * @ingroup hazo
 */
template <typename... X>
using basic_seq_d = basic_seq<by_data, X...>;
/**
 * @brief hazo basic sequence specialized with @ref udho::hazo::by_value policy
 * @tparam X...
 * @see basic_seq
 * @ingroup hazo
 */
template <typename... X>
using basic_seq_v = basic_seq<by_value, X...>;

#endif // __DOXYGEN__
    
}
}

#endif // UDHO_HAZO_SEQ_BASIC_H
