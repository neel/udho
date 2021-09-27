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


#ifndef UDHO_DB_PG_IO_FORM_H
#define UDHO_DB_PG_IO_FORM_H

#include <udho/db/pg/activities/activity.h>
#include <udho/db/pg/activities/subtask.h>
#include <type_traits>

namespace udho{
namespace db{
namespace pg{

namespace detail{
    
template <typename FormFieldT>
struct column_visitor{
    const FormFieldT& _field;
    
    column_visitor(const FormFieldT& field): _field(field){}
    
    template <typename ColumnT, typename std::enable_if<std::is_assignable<ColumnT&, typename FormFieldT::value_type>::value, bool>::type = true>
    void operator()(ColumnT& col){
        if(col.name() == _field.name()){
            col = _field.value();
        }
    }
    
    template <typename ColumnT, typename std::enable_if<!std::is_assignable<ColumnT&, typename FormFieldT::value_type>::value, bool>::type = true>
    void operator()(ColumnT& col){}
};
    
}
    
template <typename DerivedT, typename SuccessT, typename RowT, typename FormFieldT>
basic_activity<DerivedT, SuccessT, RowT>& operator<<(basic_activity<DerivedT, SuccessT, RowT>& activity, const FormFieldT& field){
    detail::column_visitor<FormFieldT> visitor(field);
    activity.self()->visit(visitor);
    return activity;
}

template <typename ActivityT, typename... DependenciesT, typename FormFieldT>
activities::subtask<ActivityT, DependenciesT...>& operator<<(activities::subtask<ActivityT, DependenciesT...>& subtask, const FormFieldT& field){
    subtask.activity() << field;
    return subtask;
}

}
}
}

#endif // UDHO_DB_PG_IO_FORM_H
