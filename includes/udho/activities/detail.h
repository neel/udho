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
    labeled(): _initialized(false) {}
    /**
     * @brief Construct a new labeled object
     * 
     * @param other 
     */
    labeled(const labeled<ActivityT, ResultT>& other): _result(other._result), _initialized(other._initialized) {}
    /**
     * @brief Construct a new labeled object
     * 
     * @param res 
     */
    labeled(const result_type& res): _result(res), _initialized(true){}
    /**
     * @brief Result can be assigned to a labeled result
     * 
     * @param res ResultT
     * @return self_type& 
     */
    self_type& operator=(const result_type& res) { 
        _result = res; 
        _initialized = true; 
        return *this; 
    }
    /**
     * @brief get the result
     * 
     * @return ResultT 
     */
    const result_type& get() const { return _result;}
    /**
     * @brief Conversion operator overload
     * 
     * @return ResultT 
     */
    operator result_type() const { return get(); }
    /**
     * @brief is initialized
     * 
     * @return true 
     * @return false 
     */
    bool initialized() const { return _initialized; }
    
    private:
        result_type _result;
        bool        _initialized;
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
    
}

}
}

#endif // UDHO_ACTIVITIES_DETAIL_H