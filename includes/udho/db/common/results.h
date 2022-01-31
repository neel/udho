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

#ifndef WEE_ACTIVITY_DB_COMMON_RESULTS_H
#define WEE_ACTIVITY_DB_COMMON_RESULTS_H

#include <vector>

namespace udho{
namespace db{

/**
 * @brief wrapper around a multi row result.
 * Intended to be used for queries where 0 or more record(s) are expected to be in the resultset.
 * @ingroup db
 * @tparam DataT specifies schema of a row
 */
template <typename DataT>
struct results{
    typedef DataT data_type;
    typedef std::vector<data_type> collection_type;
    /**
     * @brief const iterator to the records
     * 
     */
    typedef typename collection_type::const_iterator iterator;
       
    /**
     * @brief const iterator to the begin of the records
     * 
     * @return iterator 
     */
    iterator begin() const { return _rows.cbegin(); }
    /**
     * @brief const iterator to the end of the records
     * 
     * @return iterator 
     */
    iterator end() const { return _rows.cend(); }
    /**
     * @brief number of records
     * 
     * @return std::size_t 
     */
    std::size_t count() const { return _rows.size(); }
    /**
     * @brief checks for emptyness
     * 
     * @return true 
     * @return false 
     */
    bool empty() const { return !std::distance(begin(), end()); }
    /**
     * @brief const reference to the first record from the results
     * @warning Must check for emptyness using @ref empty method
     * @return const data_type& 
     */
    const data_type& front() const { return _rows.front(); }
    /**
     * @brief const reference to the last record from the results
     * @warning Must check for emptyness using @ref empty method
     * @return const data_type& 
     */
    const data_type& back() const { return _rows.back(); }
    /**
     * @brief const reference to a value of the given column of the first row
     * 
     * @tparam T 
     * @param col 
     * @return const auto& 
     */
    template <typename T>
    const auto& first(const T& col) const { return front()[col]; }
    /**
     * @brief const reference to a value of the given column of the last row
     * 
     * @tparam T 
     * @param col 
     * @return const auto& 
     */
    template <typename T>
    const auto& last(const T& col) const { return back()[col]; }
    /**
     * @brief returns a back inserter iterator
     * 
     * @return auto 
     */
    auto inserter() { return std::back_inserter(_rows); }
    
    private:
        collection_type _rows;
};

/**
 * @brief operator to write a record to the results
 * 
 * @tparam DataT 
 * @param res 
 * @param data 
 * @return results<DataT>& 
 * @ingroup db
 */
template <typename DataT>
results<DataT>& operator<<(results<DataT>& res, const DataT& data){
    *res++ = data;
    return res;
}

}
}

#endif // WEE_ACTIVITY_DB_COMMON_RESULTS_H
