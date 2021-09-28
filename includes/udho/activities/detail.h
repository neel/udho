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

#ifndef UDHO_ACTIVITIES_DETAIL_H
#define UDHO_ACTIVITIES_DETAIL_H

#include <string>

namespace udho{
namespace activities{

namespace detail{
    
/**
 * @brief Labels a result type with an activity type
 * Multiple Activities may yield the same result. In order to have activity chains that contain multiple such
 * activities, the result types are labeled with the activity type while stored in the heterogenous container.
 * @tparam ActivityT 
 * @tparam ResultT 
 */
template <typename ActivityT, typename ResultT>
struct labeled{
    typedef ActivityT activity_type;
    typedef ResultT result_type;
    typedef labeled<ActivityT, ResultT> self_type;
    
    /**
     * @brief Construct a new labeled object
     * 
     */
    labeled(){}
    /**
     * @brief Construct a new labeled object
     * 
     * @param res 
     */
    labeled(const result_type& res): _result(res){}
    /**
     * @brief Result can be assigned to a labeled result
     * 
     * @param res ResultT
     * @return self_type& 
     */
    self_type& operator=(const result_type& res) { _result = res; return *this; }
    /**
     * @brief get the result
     * 
     * @return ResultT 
     */
    result_type get() const { return _result;}
    /**
     * @brief Conversion operator overload
     * 
     * @return ResultT 
     */
    operator result_type() const { return get(); }
    
    private:
        result_type _result;
};

/**
 * @brief Checks whether the type is labeled or not
 * 
 * @tparam T 
 */
template <typename T>
struct is_labeled{
    static constexpr bool value = false;
};

/**
 * @brief Checks whether the type is labeled or not
 * 
 * @tparam T 
 */
template <typename ActivityT, typename ResultT>
struct is_labeled<labeled<ActivityT, ResultT>>{
    static constexpr bool value = true;
};

/**
 * \defgroup data data
 * data collected by activities
 * @ingroup activities
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
    
}

}
}

#endif // UDHO_ACTIVITIES_DETAIL_H