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

#ifndef WEE_ACTIVITY_DB_COMMON_RESULT_H
#define WEE_ACTIVITY_DB_COMMON_RESULT_H

namespace udho{
namespace db{

/**
 * @brief wrapper around a single row result.
 * Intended to be used for queries where 0 or 1 record is expected to be in the resultset.
 * @ingroup db
 * @tparam DataT specifies schema of a row
 */
template <typename DataT>
struct result{
    typedef DataT data_type;
       
    /**
     * @brief Construct a new result object
     * @note marks the result as empty which is changed once a value is set
     */
    result(): _empty(true){}

    /**
     * @brief get the const reference to the record inside the result
     * 
     * @return const data_type& 
     */
    const data_type& get() const { return _result; }
    /**
     * @brief get pointer to the record inside the result
     * @see get 
     * @return const data_type* 
     */
    const data_type* operator->() const { return &get(); }
    /**
     * @brief dereference operator to get the const reference to the record inside the result
     * @see get 
     * @return const data_type& 
     */
    const data_type& operator*() const  { return get(); }
    /**
     * @brief check for emptyness
     * 
     * @return true 
     * @return false 
     */
    bool empty() const { return _empty; }
    
    /**
     * @brief returns const reference to the value for the specified column
     * 
     * @tparam T 
     * @param arg 
     * @return const auto& 
     */
    template <typename T>
    const auto& operator[](const T& arg) const { return _result[arg]; }
    
    /**
     * @brief sets the record in teh result
     * 
     * @param result 
     */
    void operator()(const data_type& result){ _result = result; _empty = false; }
       
    private:
        data_type _result;
        bool      _empty;
};

/**
 * @brief operator to write a record to result
 * 
 * @tparam DataT 
 * @param res 
 * @param data 
 * @return result<DataT>& 
 * @ingroup db
 */
template <typename DataT>
result<DataT>& operator<<(result<DataT>& res, const DataT& data){
    res(data);
    return res;
}

}
}

#endif // WEE_ACTIVITY_DB_COMMON_RESULT_H
