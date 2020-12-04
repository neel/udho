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

#include <type_traits>

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
    void set(const head_type& value) { _head = value; }
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

/**
 * level<A, level<B, level<C>, level<D, void>>>
 */
template <typename HeadT, typename TailT = void>
struct level: level<typename TailT::value_type, typename TailT::tail_type>{
    typedef HeadT value_type;
    typedef level<typename TailT::value_type, typename TailT::tail_type> tail_type;
    typedef stem<HeadT> stem_type;
    
    enum { depth = tail_type::depth +1 };
    
    level(const value_type& h): _stem(h) {}
    template <typename... T>
    level(const value_type& h, const T&... ts):  tail_type(ts...), _stem(h) {}
    using tail_type::tail_type;
    
    const value_type& value() const { return _stem.head(); }
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
    
    enum { depth = 0 };
    
    level(): _stem(){}
    level(const value_type& h): _stem(h) {}
    
    const value_type& value() const { return _stem.head(); }
    
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
    
    template <typename FunctionT, typename InitialT>
    auto fold(FunctionT f, InitialT initial) const {
        return f(value(), initial);
    }
    
    stem_type _stem;

};

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
    
    using level_type::level_type;
    tree(const H& h, const T& t, const X&... xs): level<H, tree<T, X...>>(h, t, xs...){}
};

template <typename H>
struct tree<H, void>: level<H, void>{
    typedef level<H, void> level_type;
    
    using level_type::level_type;
    tree(const H& h): level<H, void>(h){}
};

}
}
}
}

#endif // UDHO_UTIL_FOLDING_H
