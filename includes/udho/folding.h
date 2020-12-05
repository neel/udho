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

#ifndef UDHO_UTIL_FOLDING_H
#define UDHO_UTIL_FOLDING_H

#include <utility>
#include <iostream>
#include <type_traits>
#include <boost/hana.hpp>

namespace udho{
namespace util{
namespace detail{
namespace folding{
    
template <typename HeadT>
struct stem{
    typedef HeadT head_type;
    
    head_type _head;
    
    stem(): _head(head_type()){};
    stem(const stem<HeadT>&) = default;
    stem(const head_type& h): _head(h){}
    const head_type& head() const { return _head; }
    head_type& head() { return _head; }
    bool operator==(const stem<HeadT>& other) const { return _head == other._head; }
    bool operator!=(const stem<HeadT>& other) const { return !operator==(other); }
    void set(const head_type& value) { _head = value; }
    operator head_type() const { return _head; }
    template <typename FunctionT>
    auto call(FunctionT f) const {
        return f(_head);
    }
    template <typename FunctionT>
    auto call(FunctionT f) {
        return f(_head);
    }
};

template <int N>
struct stem<char[N]>{
    typedef std::string head_type;
    
    head_type _head;
    
    stem(): _head(head_type()){};
    stem(const stem<char[N]>&) = default;
    stem(const char* h): _head(h){}
    const head_type& head() const { return _head; }
    head_type& head() { return _head; }
    bool operator==(const stem<char[N]>& other) const { return _head == other._head; }
    bool operator!=(const stem<char[N]>& other) const { return !operator==(other); }
    void set(const head_type& value) { _head = value; }
    operator head_type() const { return _head; }
    template <typename FunctionT>
    auto call(FunctionT f) const {
        return f(_head);
    }
    template <typename FunctionT>
    auto call(FunctionT f) {
        return f(_head);
    }
};

template <typename T>
struct proxy;

template <typename HeadT>
struct proxy<stem<HeadT>>{
    typedef HeadT value_type;
    typedef stem<HeadT> stem_type;
    typedef proxy<stem<HeadT>> self_type;
    
    stem_type& _stem;
    
    proxy(stem_type& st): _stem(st){}
    proxy(const self_type& other) = default;
    self_type& operator=(const value_type& v){
        _stem.set(v);
        return *this;
    }
    value_type& get(){
        return _stem.head();
    }
    const value_type& get() const{
        return _stem.head();
    }
    operator value_type() const {
        return get();
    }
    const value_type& operator*() const{
        return get();
    }
};

template <typename LevelT, std::size_t Index>
struct value{
    static auto apply(const LevelT& level){
        return level.template get<Index>();
    }
};

// http://loungecpp.wikidot.com/tips-and-tricks%3aindices
template <std::size_t... Is>
struct indices {};

template <std::size_t N, std::size_t... Is>
struct build_indices
    : build_indices<N-1, N-1, Is...> {};

template <std::size_t... Is>
struct build_indices<0, Is...> : indices<Is...> {
    typedef indices<Is...> indices_type;
};

template <typename LevelT, typename Indecies>
struct call_helper;
template <typename LevelT, typename Indecies>
struct access_helper;

/**
 * level<A, level<B, level<C>, level<D, void>>>
 */
template <typename HeadT, typename TailT = void>
struct level: level<typename TailT::value_type, typename TailT::tail_type>{
    typedef HeadT value_type;
    typedef level<typename TailT::value_type, typename TailT::tail_type> tail_type;
    typedef stem<HeadT> stem_type;
    typedef level<HeadT, TailT> self_type;
    
    enum { depth = tail_type::depth +1 };
    
    level(const value_type& h): _stem(h) {}
    template <typename... T>
    level(const value_type& h, const T&... ts):  tail_type(ts...), _stem(h) {}
    using tail_type::tail_type;
    
    stem_type& front() { return _stem; }
    const stem_type& front() const { return _stem; }
    tail_type& tail() { return static_cast<tail_type&>(*this); }
    const tail_type& tail() const { return static_cast<const tail_type&>(*this); }
    const value_type& value() const { return _stem.head(); }
    value_type& value() { return _stem.head(); }
    
    template <typename LevelT>
    constexpr typename std::enable_if<LevelT::depth != depth || !std::is_same<typename LevelT::value_type, value_type>::value, bool>::type operator==(const LevelT&) const { return false; }
    template <typename LevelT>
    constexpr typename std::enable_if<LevelT::depth == depth && std::is_same<typename LevelT::value_type, value_type>::value, bool>::type operator==(const LevelT& other) const {
        return _stem == other._stem && tail_type::operator==(other.tail());
    }
    template <typename LevelT>
    constexpr bool operator!=(const LevelT& other) const{
        return !(*this == other);
    }
    template <typename LevelT>
    constexpr typename std::enable_if<LevelT::depth != depth || !std::is_same<typename LevelT::value_type, value_type>::value, bool>::type less(const LevelT&) const { return false; }
    template <typename LevelT>
    constexpr typename std::enable_if<LevelT::depth == depth && std::is_same<typename LevelT::value_type, value_type>::value, bool>::type less(const LevelT& other) const {
        return _stem < other._stem && tail_type::less(other.tail());
    }
    
    template <typename T>
    bool exists() const{
        return std::is_same<value_type, T>::value || tail_type::template exists<T>();
    }
    
    template <typename T>
    bool set(const T& v, bool all = false){
        bool success = false;
        if(std::is_same<value_type, T>::value){
            _stem.set(v);
            success = true;
        }
        if(all) 
            return success || tail_type::template set<T>(v, all);
        else
            return success && tail_type::template set<T>(v, all);
    }
    
    template <int N, typename T>
    const typename std::enable_if<N == 0, bool>::type set(const T& v){
        _stem.set(v);
        return true;
    }
    template <int N, typename T>
    const typename std::enable_if<N != 0, bool>::type set(const T& v){
        return tail_type::template set<N-1, T>(v);
    }
    
    template <typename T>
    const typename std::enable_if<std::is_same<T, value_type>::value, T>::type& get() const{
        return value();
    }
    template <typename T>
    const typename std::enable_if<!std::is_same<T, value_type>::value, T>::type& get() const{
        return tail_type::template get<T>();
    }
    
    template <int N, typename = typename std::enable_if<N == 0, value_type>::type>
    const value_type& get() const {
        return value();
    }
    template <int N, typename = typename std::enable_if<N != 0, value_type>::type>
    const auto& get() const {
        return tail_type::template get<N-1>();
    }
    
    template <int N, typename = typename std::enable_if<N == 0, value_type>::type>
    proxy<stem_type> at(){
        return proxy<stem_type>(_stem);
    }
    template <int N, typename = typename std::enable_if<N != 0, value_type>::type>
    auto at(){
        return tail_type::template at<N-1>();
    }
    template <typename T, typename = typename std::enable_if<std::is_same<T, value_type>::value, proxy<stem_type>>::type>
    proxy<stem_type> at(){
        return proxy<stem_type>(_stem);
    }
    template <typename T, typename = typename std::enable_if<!std::is_same<T, value_type>::value, proxy<stem_type>>::type>
    auto at(){
        return tail_type::template at<T>();
    }
    
    template <int N, typename = typename std::enable_if<N == 0, tail_type>::type>
    tail_type& tail_at() { return tail(); }
    template <int N, typename = typename std::enable_if<N != 0>::type>
    auto& tail_at() { return tail_type::template tail_at<N>(); }
    template <int N, typename = typename std::enable_if<N == 0, tail_type>::type>
    const tail_type& tail_at() const { return tail(); }
    template <int N, typename = typename std::enable_if<N != 0>::type>
    auto& tail_at() const { return tail_type::template tail_at<N>(); }
        
    template <typename FunctionT>
    void visit(FunctionT& f) const{
        _stem.call(f);
        tail_type::visit(f);
    }
    template <typename FunctionT>
    void visit(FunctionT& f){
        _stem.call(f);
        tail_type::visit(f);
    }
       
    template <typename FunctionT, typename InitialT>
    auto fold(FunctionT f, InitialT initial) const {
        return f(value(), tail_type::fold(f, initial));
    }
    
    stem_type _stem;
};

/**
 * folding_level<D, void>
 */
template <typename HeadT>
struct level<HeadT, void>{
    typedef HeadT value_type;
    typedef void tail_type;
    typedef stem<HeadT> stem_type;
    typedef level<HeadT, void> self_type;
    
    enum { depth = 0 };
    
    level(): _stem(){}
    level(const value_type& h): _stem(h) {}
    
    stem_type& front() { return _stem; }
    const stem_type& front() const { return _stem; }
    const value_type& value() const { return _stem.head(); }
    value_type& value() { return _stem.head(); }
    
    template <typename LevelT>
    constexpr typename std::enable_if<LevelT::depth != depth || !std::is_same<typename LevelT::value_type, value_type>::value, bool>::type operator==(const LevelT&) const { return false; }
    template <typename LevelT>
    constexpr typename std::enable_if<LevelT::depth == depth && std::is_same<typename LevelT::value_type, value_type>::value, bool>::type operator==(const LevelT& other) const {
        return _stem == other._stem;
    }
    template <typename LevelT>
    constexpr bool operator!=(const LevelT& other) const{
        return !(*this == other);
    }
    template <typename LevelT>
    constexpr typename std::enable_if<LevelT::depth != depth || !std::is_same<typename LevelT::value_type, value_type>::value, bool>::type less(const LevelT&) const { return false; }
    template <typename LevelT>
    constexpr typename std::enable_if<LevelT::depth == depth && std::is_same<typename LevelT::value_type, value_type>::value, bool>::type less(const LevelT& other) const {
        return _stem < other._stem;
    }
    
    template <typename T>
    bool exists() const{
        return std::is_same<HeadT, T>::value;
    }
    
    template <typename T>
    bool set(const T& v, bool){
        if(std::is_same<HeadT, T>::value){
            _stem.template set<T>(v);
            return true;
        }
        return false;
    }
    
    template <int N, typename T>
    const typename std::enable_if<N == 0 && std::is_same<T, value_type>::value, bool>::type set(const T& v){
        _stem.set(v);
        return true;
    }
    
    template <typename T>
    const typename std::enable_if<std::is_same<T, value_type>::value, T>::type& get() const{
        return value();
    }
    template <int N, typename = typename std::enable_if<N == 0, value_type>::type>
    const value_type& get() const {
        return value();
    }
    
    template <int N, typename = typename std::enable_if<N == 0, value_type>::type>
    proxy<stem_type> at(){
        return proxy<stem_type>(_stem);
    }
    template <typename T>
    typename std::enable_if<std::is_same<T, value_type>::value, proxy<stem_type>>::type at(){
        return proxy<stem_type>(_stem);
    }

    template <typename FunctionT>
    void visit(FunctionT& f) const{
        _stem.call(f);
    }
    template <typename FunctionT>
    void visit(FunctionT& f){
        _stem.call(f);
    }
    template <typename FunctionT>
    void operator()(FunctionT&& f){
        f(value());
    }
    template <typename FunctionT, typename InitialT>
    auto fold(FunctionT f, InitialT initial) const {
        return f(value(), initial);
    }
    
    stem_type _stem;
};

template <typename LevelT, std::size_t N>
struct level_at_proxy_helper{
    auto operator()(LevelT& level){
        return level.template at<N>();
    }
};

template <typename HeadT, typename TailT, std::size_t... Is>
struct access_helper<level<HeadT, TailT>, indices<Is...>>{
    typedef level<HeadT, TailT> level_type;
    
//     level_type& _level;
//     access_helper(level_type& l): _level(l){}
    
    constexpr auto apply(){
        std::cout << "QQQQ" << std::endl;
        return boost::hana::make_tuple(
            boost::hana::make_pair(boost::hana::int_c<Is>, level_at_proxy_helper<level_type, Is>{})...
        );
    }
};


// template <typename LevelL, typename LevelR>
// struct comparator{};
// 
// template <typename HL, typename TL, typename HR, typename TR>
// struct comparator<level<HL, TL>, level<HR, TR>>{
//     typedef level<HL, TL> left_type;
//     typedef level<HR, TR> right_type;
//     
//     bool operator==(const left_type& l, const right_type& r) const{
//         return l == r;
//     }
// };

// template <typename HL, typename TL, typename HR, typename TR>
// bool operator==(const level<HL, TL>& l, const level<HR, TR>& r){
//     return l == r;
// }
// 
// template <typename HL, typename TL, typename HR, typename TR>
// bool operator!=(const level<HL, TL>& l, const level<HR, TR>& r){
//     return !operator==(l, r);
// }

template <typename HeadT, typename TailT, std::size_t... Is>
struct call_helper<level<HeadT, TailT>, indices<Is...>>{
    typedef level<HeadT, TailT> level_type;
    
    const level_type& _level;
    call_helper(const level_type& l): _level(l){}
    
    template <typename FunctionT>
    auto apply(FunctionT&& f){
        return f(value<level_type, Is>::apply(_level)...);
    }
};

struct udho_folding_tree_tag{};

/**
 * tree<A, B, C, D>: level<A, tree<B, C, D>>                                                            -> level<A, level<B, level<C, level<D, void>>>>     -> stem<A>  depth 3
 *                            tree<B, C, D> : level<B, tree<C, D>>                                      -> level<B, level<C, level<D, void>>>               -> stem<B>  depth 2
 *                                                     tree<C, D>: level<C, tree<D>>                    -> level<C, level<D, void>>                         -> stem<C>  depth 1
 *                                                                          tree<D>: level<D, void>     -> level<D, void>                                   -> stem<D>  depth 0
 * 
 * \code
    tree<int, std::string, double, int> vec(42, "Hello", 3.14, 84);
    
    std::cout << vec.get<int>() << std::endl;
    std::cout << vec.get<std::string>() << std::endl;
    std::cout << vec.get<double>() << std::endl;
    std::cout << "-----" << std::endl;
    std::cout << vec.get<0>() << std::endl;
    std::cout << vec.get<1>() << std::endl;
    std::cout << vec.get<2>() << std::endl;
    std::cout << vec.get<3>() << std::endl;
    std::cout << "-----" << std::endl;
    std::cout << vec.set<0>(43) << std::endl;
    std::cout << vec.set<1>("World") << std::endl;
    std::cout << vec.set<2>(6.28) << std::endl;
    std::cout << vec.set<3>(42) << std::endl;
    std::cout << "-----" << std::endl;
    std::cout << vec.get<0>() << std::endl;
    std::cout << vec.get<1>() << std::endl;
    std::cout << vec.get<2>() << std::endl;
    std::cout << vec.get<3>() << std::endl;
 * \endcode
 */
template <typename H, typename T = void, typename... X>
struct tree: level<H, tree<T, X...>>{
    typedef level<H, tree<T, X...>> level_type;
    
    using hana_tag = udho_folding_tree_tag;
    
    using level_type::level_type;
    tree(const H& h, const T& t, const X&... xs): level<H, tree<T, X...>>(h, t, xs...){}
    template <typename FunctionT>
    auto unpack(FunctionT&& f) const{
        call_helper<level_type, typename build_indices<2+sizeof...(X)>::indices_type> helper(*this);
        return helper.apply(std::forward<FunctionT>(f));
    }
    
    struct hana_accessors_impl {
        static BOOST_HANA_CONSTEXPR_LAMBDA auto apply() {
            access_helper<level_type, typename build_indices<2+sizeof...(X)>::indices_type> helper;
            return helper.apply();
        }
    };
};

// template <typename L, typename R>
// struct comparator{
//     bool lt(const L&, const R&) const { return false; }
// };
// template <typename H, typename... T>
// struct comparator<tree<H, T...>, tree<H, T...>>{
//     bool lt(const tree<H, T...>& l, const tree<H, T...>& r) const { return l < r; }
// };

// template <typename L, typename R>
// bool operator==(const L& l, const R& r){
//     comparator<L, R> cmp;
//     return cmp.lt(l, r);
// }
// template <typename L, typename R>
// bool operator!=(const L& l, const R& r){
//     return !operator==(l, r);
// }

template <typename H>
struct tree<H, void>: level<H, void>{
    typedef level<H, void> level_type;
    
    using hana_tag = udho_folding_tree_tag;
    
    using level_type::level_type;
    tree(const H& h): level<H, void>(h){}
    template <typename FunctionT>
    auto unpack(FunctionT&& f) const{
        call_helper<level_type, typename build_indices<1>::indices_type> helper(*this);
        return helper.apply(std::forward<FunctionT>(f));
    }
    
    struct hana_accessors_impl {
        static BOOST_HANA_CONSTEXPR_LAMBDA auto apply() {
            access_helper<level_type, typename build_indices<1>::indices_type> helper;
            return helper.apply();
        }
    };
};

template <typename... X>
tree<X...> make(const X&... xs){
    return tree<X...>(xs...);
}

}
}
}
}

namespace boost {
namespace hana {
        
//     template <>
//     struct equal_impl<udho_folding_tree_tag, udho_folding_tree_tag, when<true>>{ 
//         template <typename X, typename Y>
//         static constexpr auto apply(X const&, Y const&) {
//             return hana::false_c;
//         }
//     };
    
    template <>
    struct at_impl<udho::util::detail::folding::udho_folding_tree_tag> {
        template <typename Xs, typename N>
        static constexpr decltype(auto) apply(Xs&& xs, N const&) {
            return xs.template at<N::value>();
        }
    };
    
//     template <typename... T>
//     struct accessors_impl<udho::util::detail::folding::tree<T...>> {
//         static BOOST_HANA_CONSTEXPR_LAMBDA auto apply() {
// 
//         }
//     };

    template <>
    struct drop_front_impl<udho::util::detail::folding::udho_folding_tree_tag> {
        template <typename Xs, typename N>
        static constexpr auto apply(Xs&& xs, N const&) {
            return xs.template tail_at<N::value>();
        }
    };

    template <>
    struct is_empty_impl<udho::util::detail::folding::udho_folding_tree_tag> {
        template <typename Xs>
        static constexpr auto apply(Xs const& xs) {
            return xs.depth == 1;
        }
    };
    
    template <>
    struct unpack_impl<udho::util::detail::folding::udho_folding_tree_tag> {
        template <typename Xs, typename F>
        static constexpr auto apply(Xs&& xs, F&& f) {
            return xs.unpack(std::forward<F>(f));
        }
    };
    
    template <>
    struct make_impl<udho::util::detail::folding::udho_folding_tree_tag> {
        template <typename ...Args>
        static constexpr auto apply(Args&& ...args) {
            return udho::util::detail::folding::tree<Args...>(std::forward<Args>(args)...);
        }
    };
    
    template <>
    struct Sequence<udho::util::detail::folding::udho_folding_tree_tag> : std::true_type { };
    
}
}

#endif // UDHO_UTIL_FOLDING_H
