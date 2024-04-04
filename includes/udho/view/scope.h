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

#ifndef UDHO_VIEW_SCOPE_H
#define UDHO_VIEW_SCOPE_H

#include <string>
#include <utility>
#include <type_traits>

namespace udho{
namespace view{
namespace data{

namespace detail{

template <typename T, typename Enable = void>
struct value;

template <typename T>
struct value<T, std::enable_if_t< !std::is_reference_v<T> && !std::is_pointer_v<T> >>{
    using type                  = T;
    using reference_type        = type &;
    using const_reference_type  = const type &;

    value(T&& v): _v(std::move(v)) {}
    const_reference_type v() const { return _v; }
    const_reference_type operator*() const { return v(); }
    private:
        T _v;
};

template <typename T>
struct value<T, std::enable_if_t< std::is_reference_v<T> && std::is_const_v<std::remove_reference_t<T>> >>{
    using type                  = std::remove_reference_t<std::remove_const_t<T>>;
    using reference_type        = type &;
    using const_reference_type  = const type &;

    value(T v): _v(v) {}
    const_reference_type v() const { return _v; }
    const_reference_type operator*() const { return v(); }
    private:
        T _v;
};

template <typename T>
struct value<T, std::enable_if_t< std::is_reference_v<T> && !std::is_const_v<std::remove_reference_t<T>> >>{
    using type                  = std::remove_reference_t<T>;
    using reference_type        = type &;
    using const_reference_type  = const type &;

    value(T v): _v(v) {}
    const_reference_type v() const { return _v; }
    reference_type v() { return _v; }
    const_reference_type operator*() const { return v(); }
    reference_type operator*() { return v(); }
    private:
        T _v;
};

}

template <typename K, typename T, typename Enable = void>
struct nvp;

template <typename K, typename T>
struct nvp<K, T, std::enable_if_t< std::is_const_v<std::remove_reference_t<T>> > >{
    using name_type             = K;
    using value_type            = detail::value<T>;
    using reference_type        = typename value_type::reference_type;
    using const_reference_type  = typename value_type::const_reference_type;

    nvp(name_type&& name, T&& v): _name(std::move(name)), _value(std::forward<T>(v)) {}
    const name_type& name() const { return _name; }

    const_reference_type operator*() const { return *_value; }

    friend std::ostream& operator<< (std::ostream& stream, const nvp& nvp) {
        stream << nvp._name << ": " << *nvp._value;
        return stream;
    }
    private:
        name_type  _name;
        value_type _value;
};

template <typename K, typename T>
struct nvp<K, T, std::enable_if_t< !std::is_const_v<std::remove_reference_t<T>> > >{
    using name_type             = K;
    using value_type            = detail::value<T>;
    using reference_type        = typename value_type::reference_type;
    using const_reference_type  = typename value_type::const_reference_type;

    nvp(name_type&& name, T&& v): _name(std::move(name)), _value(std::forward<T>(v)) {}
    const name_type& name() const { return _name; }
    const_reference_type operator*() const { return *_value; }
    reference_type operator*(){ return *_value; }

    friend std::ostream& operator<< (std::ostream& stream, const nvp& nvp) {
        stream << nvp._name << ": " << *nvp._value;
        return stream;
    }
    private:
        name_type _name;
        value_type  _value;
};


/**
 * @brief make name value pair of a reference (or a ...)
 * @tparam name string name
 * @tparam v value
 *
 */
template <typename K, typename T>
nvp<K, T> make_nvp(K&& name, T&& v){
    return nvp<K, T>(std::move(name), std::forward<T>(v));
}

// iterators

template <typename K, typename IteratorT, typename Enable = void>
struct nip {
    using name_type             = K;
    using iterator_type         = IteratorT;
    using iterator_traits       = std::iterator_traits<iterator_type>;
    using value_type            = typename iterator_traits::value_type;
    using difference_type       = typename iterator_traits::difference_type;
    using size_type             = difference_type;
    using reference_type        = typename iterator_traits::reference;
    using const_reference_type  = const typename iterator_traits::reference;

    nip(name_type&& name, iterator_type begin, iterator_type end): _name(std::move(name)), _begin(begin), _end(end) {}

    iterator_type begin() { return _begin; }
    iterator_type end() { return _end; }

    size_type size() const { return std::distance(_begin, _end); }

    private:
        name_type     _name;
        iterator_type _begin, _end;
};

template <typename K, typename IteratorT>
struct nip<K, IteratorT, std::enable_if_t<std::is_same<typename std::iterator_traits<IteratorT>::iterator_category, std::random_access_iterator_tag>::value>> {
    using name_type             = K;
    using iterator_type         = IteratorT;
    using iterator_traits       = std::iterator_traits<iterator_type>;
    using value_type            = typename iterator_traits::value_type;
    using difference_type       = typename iterator_traits::difference_type;
    using size_type             = difference_type;
    using reference_type        = typename iterator_traits::reference;
    using const_reference_type  = const typename iterator_traits::reference;

    nip(name_type&& name, iterator_type begin, iterator_type end): _name(std::move(name)), _begin(begin), _end(end) {}

    iterator_type begin() { return _begin; }
    iterator_type end() { return _end; }

    size_type size() const { return (_end - _begin);}
    reference_type operator[](size_type index) { return *(_begin + index); }
    const_reference_type operator[](size_type index) const { return *(_begin + index); }

    private:
        name_type     _name;
        iterator_type _begin, _end;
};

template <typename K, typename IteratorT>
nip<K, IteratorT> make_nvp(K&& name, IteratorT begin, IteratorT end){
    return nip<K, IteratorT>(std::move(name), begin, end);
}

}
}
}

#endif // UDHO_VIEW_SCOPE_H
