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

#ifndef WEE_ACTIVITY_DB_COMMON_DETAIL_H
#define WEE_ACTIVITY_DB_COMMON_DETAIL_H

#include <functional>
#include <udho/db/common/results.h>
#include <udho/db/common/result.h>

namespace udho{
namespace db{

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
struct processor<ThatT, db::results<DataT>>{
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
struct processor<ThatT, db::result<DataT>>{
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

}
}

#endif // WEE_ACTIVITY_DB_COMMON_DETAIL_H
