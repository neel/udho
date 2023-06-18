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

#ifndef UDHO_DB_PG_ACTIVITY_H
#define UDHO_DB_PG_ACTIVITY_H

#include <udho/db/pg/activities/basic.h>

namespace udho{
namespace db{
namespace pg{
    
/**
 * @brief PostgreSQL activity for asynchronous database queries
 * - A PostgreSQL activity `X` must derive from `pg::activity<X>`
 * - The derived class `X` must provide an operator() overload
 * - The overloaded operator() must call `pg::activity<X>::query(SQL)` with an OZO query
 * - If it is required to consider the activity as successful without querying then use the `pg::activity<X>::pass()`  method with an appropriate success result
 * 
 * @tparam DerivedT The Derived activity class 
 * @tparam SuccessT Type of success result, defaults to db::none, If the provided SuccessT has a typedef `data_type` then that SuccessT is used to store the successful result(s) of the activity. Otherwise the type db::result<SuccessT> is used
 * @tparam RowT By default RowT is  same as SuccessT. However usercode may provide a custom RowT
 * @ingroup pg 
 */
template <typename DerivedT, typename SuccessT = db::none, typename RowT = typename std::conditional<db::detail::HasDataType<SuccessT>::value, SuccessT, db::result<SuccessT>>::type::data_type>
struct activity: basic_activity<DerivedT, SuccessT, RowT>{
    typedef basic_activity<DerivedT, SuccessT, RowT> base;
    typedef typename base::success_type success;
    
    using base::base;
    
    void operator()(const typename base::success_type& data){
        pass(data);
        base::clear();
    }
    void pass(const typename base::success_type& data){
        base::success(data);
    }
    template <typename U>
    void pass(const U& value){
        success data;
        data << value;
        pass(data);
    }
    void pass(){
        pass(typename base::success_type());
    }
};
    
}
}
}

#endif // UDHO_DB_PG_ACTIVITY_H

