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

#ifndef WEE_ACTIVITY_DB_COMMON_H
#define WEE_ACTIVITY_DB_COMMON_H

#include <chrono>
#include <iterator>
#include <sstream>
#include <type_traits>
#include <boost/iterator/transform_iterator.hpp>
#include <udho/page.h>
#include <udho/activities.h>
#include <boost/beast/http/message.hpp>
#include <boost/hana/fold.hpp>
#include <boost/hana/tuple.hpp>
#include <udho/db/fieldset.h>

namespace udho{
namespace db{
    
template <typename DataT>
struct results{
    typedef DataT data_type;
    typedef results<DataT> self_type;
    typedef std::vector<data_type> collection_type;
    typedef typename collection_type::const_iterator iterator;
       
    iterator begin() const { return _rows.cbegin(); }
    iterator end() const { return _rows.cend(); }
    std::size_t count() const { return _rows.size(); }
    bool empty() const { return !std::distance(begin(), end()); }
    const data_type& front() const { return _rows.front(); }
    const data_type& back() const { return _rows.back(); }
    template <typename T>
    const auto& first(const T& col) const { return front()[col]; }
    template <typename T>
    const auto& last(const T& col) const { return back()[col]; }
    
    auto inserter() { return std::back_inserter(_rows); }
    
    struct blank{
        bool operator()(const self_type& result) const{
            return result.empty();
        }
    };
    
    struct never{
        bool operator()(const self_type&) const{
            return false;
        }
    };
    
    private:
        collection_type _rows;
};

template <typename DataT>
struct result{
    typedef DataT data_type;
    typedef result<DataT> self_type;
       
    result(): _empty(true){}

    const data_type& get() const { return _result; }
    const data_type* operator->() const { return &get(); }
    const data_type& operator*() const  { return get(); }
    bool empty() const { return _empty; }
    
    template <typename T>
    const auto& operator[](const T& arg) const { return _result[arg]; }
    
    void operator()(const data_type& result){ _result = result; _empty = false; }
    
    struct blank{
        bool operator()(const self_type& result) const{
            return result.empty();
        }
    };
    
    struct never{
        bool operator()(const self_type&) const{
            return false;
        }
    };
       
    private:
        data_type _result;
        bool      _empty;
};

struct none{
    typedef void data_type;
    
    inline bool empty() const { return true; }
};

template <typename DataT>
result<DataT>& operator<<(result<DataT>& res, const DataT& data){
    res(data);
    return res;
}

template <typename DataT>
results<DataT>& operator<<(results<DataT>& res, const DataT& data){
    *res++ = data;
    return res;
}

template <typename DataT>
none& operator<<(none& res, const DataT&){
    return res;
}

namespace detail{

/**
 * @brief Converts an object of source type SourceT to target type TargetT using a converter ThatT.
 * ThatT should provide an operator() overload that returns a TargetT when called with a SourceT.
 * @internal However if SourceT is convertible to TargetT that ThatT is not used for conversion.
 * 
 * @tparam ThatT 
 * @tparam TargetT 
 * @tparam SourceT 
 * @tparam convertible 
 */
template <typename ThatT, typename TargetT, typename SourceT, bool convertible = std::is_convertible<SourceT, TargetT>::value>
struct conversion{
    const ThatT& _that;
    
    conversion(const ThatT& that): _that(that){}
    TargetT operator()(const SourceT& source) const{
        TargetT target = source;
        return target;
    }
};

/**
 * @brief Converts an object of source type SourceT to target type TargetT using a converter ThatT.
 * ThatT should provide an operator() overload that returns a TargetT when called with a SourceT.
 * @internal However if SourceT is convertible to TargetT that ThatT is not used for conversion.
 * 
 * @tparam ThatT 
 * @tparam TargetT 
 * @tparam SourceT 
 * @tparam convertible 
 */
template <typename ThatT, typename TargetT, typename SourceT>
struct conversion<ThatT, TargetT, SourceT, false>{
    const ThatT& _that;
    
    conversion(const ThatT& that): _that(that){}
    TargetT operator()(const SourceT& source) const{
        return _that(source);
    }
};

/**
 * @brief An unary function object to transform an object of source type to target type
 * 
 * @tparam ThatT 
 * @tparam TargetT 
 * @tparam SourceT 
 */
template <typename ThatT, typename TargetT, typename SourceT = TargetT>
struct transform: std::unary_function<const SourceT&, TargetT>, private detail::conversion<ThatT, TargetT, SourceT>{
    typedef ThatT   that_type;
    typedef TargetT target_type;
    typedef SourceT source_type;
    typedef detail::conversion<ThatT, TargetT, SourceT> converter_type;
    
    transform(const that_type& that): converter_type(that){}
    target_type operator()(const source_type& source) const{
        return converter_type::operator()(source);
    }
};

/**
 * @brief When the SourceT and TargetT are same
 * 
 * @tparam ThatT 
 * @tparam TargetT 
 */
template <typename ThatT, typename TargetT>
struct transform<ThatT, TargetT, TargetT>: std::unary_function<const TargetT&, TargetT>{
    typedef ThatT   that_type;
    typedef TargetT target_type;
    typedef TargetT source_type;
    
    const that_type& _that;
    
    transform(const that_type& that): _that(that){}
    target_type operator()(const source_type& source) const{
        return source;
    }
};

template <typename ThatT, typename SuccessT>
struct processor{
    typedef ThatT    that_type;
    typedef SuccessT success_type;
    
    that_type& _that;
    
    processor(that_type& that): _that(that){}
    template <typename IteratorT>
    void process(IteratorT begin, IteratorT end){
        _that(begin, end);
    }
};
template <typename ThatT, typename DataT>
struct processor<ThatT, results<DataT>>{
    typedef ThatT    that_type;
    typedef results<DataT> success_type;
    
    that_type& _that;
    
    processor(that_type& that): _that(that){}
    template <typename IteratorT>
    void process(IteratorT begin, IteratorT end){
        success_type data;
        std::copy(begin, end, data.inserter());
        _that(data);
    }
};

template <typename ThatT, typename DataT>
struct processor<ThatT, result<DataT>>{
    typedef ThatT    that_type;
    typedef result<DataT> success_type;
    
    that_type& _that;
    
    processor(that_type& that): _that(that){}
    template <typename IteratorT>
    void process(IteratorT begin, IteratorT end){
        success_type data;
        if(std::distance(begin, end) > 0){
            data(*begin);
        }
        _that(data);
    }
};

template< typename T >
struct HasDataType{
  typedef char                 yes;
  typedef struct{ char d[2]; } no;

  template<typename U>
  static yes test( typename U::data_type* );
  template<typename U>
  static no test(...);

  static const bool value = ( sizeof( test<T>(0) ) == sizeof( yes ) );
};

}

template <typename ContextT, typename SuccessT, boost::beast::http::status StatusE = boost::beast::http::status::bad_request>
struct on_error{
    ContextT _ctx;
    boost::beast::http::status _status;
    
    on_error(ContextT ctx, boost::beast::http::status status = StatusE): _ctx(ctx), _status(status){}
    ContextT& context() { return _ctx; }
    boost::beast::http::status status() const { return _status; }
    
    void operator()(const SuccessT& /*result*/){
//         if(result.empty()){
            _ctx << udho::exceptions::http_error(_status);
//         }
    }
};

template <typename TagT, typename T>
struct tag{
    typedef T value_type;
    typedef tag<TagT, T> self_type;
    
    value_type _value;
    
    tag() = default;
    tag(const value_type& v): _value(v){}
    self_type& operator=(const value_type& v){
        _value = v;
        return *this;
    }
    T get() const{
        return _value;
    }
    operator T() const{
        return get();
    }
    T operator*() const{
        return get();
    }
};

}
}

#endif // WEE_ACTIVITY_DB_COMMON_H
