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

#ifndef UDHO_DB_PG_SUBTASK_H
#define UDHO_DB_PG_SUBTASK_H

#include <udho/db/pg/activities/fwd.h>
#include <udho/activities/subtask.h>
#include <udho/activities/data.h>
#include <udho/db/pg/activities/on.h>

namespace udho{
namespace db{
namespace pg{
namespace activities{
    
/**
 * @brief A pg::subtask is the specialization of udho::activities::subtask that adds few features related to a postgresql activity 
 * - The operator[] forwards to the operator[] of teh underlying pg::activity object
 * - Automatically binds the default `if_failed`, `if_errored` and `cancel_if` callbacks
 * 
 * @tparam ActivityT 
 * @tparam DependenciesT 
 */
template <typename ActivityT, typename... DependenciesT>
struct subtask: udho::activities::subtask<ActivityT, DependenciesT...>{
    typedef udho::activities::subtask<ActivityT, DependenciesT...> subtask_base;
    typedef subtask<ActivityT, DependenciesT...> self_type;
    
    subtask() = default;
    subtask(const self_type& other) = default;
    
    /**
     * @brief Construct a pg::subtask with a controller and additional optional arguments passed to the activity's constructor
     * 
     * @tparam ContextT 
     * @tparam T... Activities
     * @tparam U... Types of additional arguments
     * @param controller 
     * @param u... Additional parameters for the activity constructor
     * @return self_type 
     */
    template <typename ContextT, typename... T, typename... U>
    static self_type with(pg::activities::controller<ContextT, T...> controller, U&&... u){
        return self_type(controller, std::forward<U>(u)...);
    }
    
    /**
     * @brief Construct a pg::subtask with a shared pointer to the collector and additional optional arguments passed to the activity's constructor
     * 
     * @tparam ContextT 
     * @tparam T... Activities
     * @tparam U... Types of additional arguments
     * @param collector_ptr 
     * @param u... Additional parameters for the activity constructor
     * @return self_type 
     */
    template <typename ContextT, typename... T, typename... U>
    static self_type with(std::shared_ptr<udho::activities::collector<ContextT, T...>> collector_ptr, U&&... u){
        return self_type(collector_ptr, std::forward<U>(u)...);
    }
    
    /**
     * @brief Get or Set the value of a field of the pg::activity
     * 
     * @tparam FieldT 
     * @param field 
     * @return decltype(auto) 
     */
    template <typename FieldT>
    decltype(auto) operator[](FieldT&& field){
        return subtask_base::activity()[std::forward<FieldT>(field)];
    }
    
    /**
     * @brief Get or Set the value of a field of the pg::activity
     * 
     * @tparam FieldT 
     * @param field 
     * @return decltype(auto) 
     */
    template <typename FieldT>
    decltype(auto) operator[](FieldT&& field) const {
        return subtask_base::activity()[std::forward<FieldT>(field)];
    }
    
    protected:
        using subtask_base::subtask_base;
        template <typename ContextT, typename... T, typename... U>
        subtask(pg::activities::controller<ContextT, T...> controller, U&&... u): subtask_base(controller.collector(), controller.pool(), controller.io(), std::forward<U>(u)...){
            subtask_base::if_failed(pg::on::failure<ActivityT>(controller.context()));
            subtask_base::if_errored(pg::on::error<ActivityT>(controller.context()));
            subtask_base::cancel_if(pg::on::validate<ActivityT>());
        }
        template <typename ContextT, typename... T, typename... U>
        subtask(std::shared_ptr<udho::activities::collector<ContextT, T...>> collector_ptr, U&&... u): subtask_base(collector_ptr, std::forward<U>(u)...){
            subtask_base::if_failed(pg::on::failure<ActivityT>(collector_ptr->context()));
            subtask_base::if_errored(pg::on::error<ActivityT>(collector_ptr->context()));
            subtask_base::cancel_if(pg::on::validate<ActivityT>());
        }
};
    
}
}
}
}

#endif // UDHO_DB_PG_SUBTASK_H
