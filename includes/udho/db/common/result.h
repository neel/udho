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

template <typename DataT>
struct result{
    typedef DataT data_type;
       
    result(): _empty(true){}

    const data_type& get() const { return _result; }
    const data_type* operator->() const { return &get(); }
    const data_type& operator*() const  { return get(); }
    bool empty() const { return _empty; }
    
    template <typename T>
    const auto& operator[](const T& arg) const { return _result[arg]; }
    
    void operator()(const data_type& result){ _result = result; _empty = false; }
    
    struct blank{
        bool operator()(const result<DataT>& result) const{
            return result.empty();
        }
    };
    
    struct never{
        bool operator()(const result<DataT>&) const{
            return false;
        }
    };
       
    private:
        data_type _result;
        bool      _empty;
};

template <typename DataT>
result<DataT>& operator<<(result<DataT>& res, const DataT& data){
    res(data);
    return res;
}

}
}

#endif // WEE_ACTIVITY_DB_COMMON_RESULT_H
