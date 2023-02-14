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

#ifndef UDHO_DB_PG_SCHEMA_BASIC_H
#define UDHO_DB_PG_SCHEMA_BASIC_H

#include <udho/hazo.h>
#include <udho/db/pg/schema/fwd.h>
#include <udho/db/pg/decorators.h>
#include <udho/db/pg/schema/constraints.h>

namespace udho{
namespace db{
namespace pg{
   
/**
 * @brief Typesafe container to define a projection.
 * @tparam Fields...
 * 
 * A schema is a type safe heterogenous container that defines a projection.
 * It is used to define the subset of columns in a query. A schema can also
 * contain values corresponding to each of these columns. Usually the fields
 * inside a schama are created using @ref PG_ELEMENT and have a PostgreSQL 
 * type associated with it.
 * 
 * It can be used as an associative container to set and get the values for 
 * each of the fields in the schema. It will raise compile time error if the 
 * requested field is not part of the schema or a value of an unexpected type
 * is being set for a field. 
 *  
 * @ingroup schema
 */
template <typename... Fields>
struct basic_schema: udho::hazo::map_d<Fields...>{
    typedef udho::hazo::map_d<Fields...> map_type;
    using map_type::map_type;

    
    template <template <typename> class ConditionT>
    using exclude_if = typename map_type::template exclude_if<ConditionT>::template translate<basic_schema>;
    template <typename... U>
    using exclude = typename map_type::template exclude<U...>::template translate<basic_schema>;
    template <typename... U>
    using include = typename map_type::template extend<U...>::template translate<basic_schema>;
    template <typename ElementT>
    using contains = typename map_type::template contains<ElementT>;
    template <typename KeyT>
    using has = typename map_type::template has<KeyT>;
    using indices = typename map_type::indices;
    using keys = typename map_type::keys;

    template <typename FieldT>
    using referenced_by = typename pg::constraints::referenced_by<FieldT, Fields...>;
    
    template <typename K>
    decltype(auto) operator()(const K& k) const { return map_type::data(k);}
    
    template <typename K>
    decltype(auto) field(const K& k){ return map_type::operator[](k);}
    template <typename K>
    decltype(auto) field(const K& k) const { return map_type::operator[](k);}
    template <typename K>
    decltype(auto) cfield(const K& k) const { return map_type::operator[](k);}
    
    auto fields() const {
        return map_type::decorate(decorators::keys{});
    }
    template <typename PrefixT>
    auto fields(PrefixT prefix) const {
        return map_type::decorate(decorators::keys::prefixed(std::move(prefix)));
    }
    template <typename... T>
    auto fields_only() const {
        return map_type::decorate(decorators::keys::only<T...>{});
    }
    template <typename... T, typename PrefixT>
    auto fields_only(PrefixT prefix) const {
        return map_type::decorate(decorators::keys::only<T...>::prefixed(std::move(prefix)));
    }
    template <typename... T>
    auto fields_except() const {
        return map_type::decorate(decorators::keys::except<T...>{});
    }
    template <typename... T, typename PrefixT>
    auto fields_except(PrefixT prefix) const {
        return map_type::decorate(decorators::keys::except<T...>::prefixed(std::move(prefix)));
    }
    
    auto unqualified_fields() const {
        return map_type::decorate(decorators::keys::unqualified{});
    }
    template <typename... T>
    auto unqualified_fields_only() const {
        return map_type::decorate(decorators::keys::unqualified::only<T...>{});
    }
    template <typename... T>
    auto unqualified_fields_except() const {
        return map_type::decorate(decorators::keys::unqualified::except<T...>{});
    }
    
    auto values() const {
        return map_type::decorate(decorators::values{});
    }
    template <typename... T>
    auto values_only() const {
        return map_type::decorate(decorators::values::only<T...>{});
    }
    template <typename... T>
    auto values_except() const {
        return map_type::decorate(decorators::values::except<T...>{});
    }
    
    auto assignments() const {
        return map_type::decorate(decorators::assignments{});
    }
    template <typename PrefixT>
    auto assignments(PrefixT prefix) const {
        return map_type::decorate(decorators::assignments::prefixed(std::move(prefix)));
    }
    template <typename... T>
    auto assignments_only() const {
        return map_type::decorate(decorators::assignments::only<T...>{});
    }
    template <typename... T, typename PrefixT>
    auto assignments_only(PrefixT prefix) const {
        return map_type::decorate(decorators::assignments::only<T...>::prefixed(std::move(prefix)));
    }
    template <typename... T>
    auto assignments_except() const {
        return map_type::decorate(decorators::assignments::except<T...>{});
    }
    template <typename... T, typename PrefixT>
    auto assignments_except(PrefixT prefix) const {
        return map_type::decorate(decorators::assignments::except<T...>::prefixed(std::move(prefix)));
    }
    
    auto unqualified_assignments() const {
        return map_type::decorate(decorators::assignments::unqualified{});
    }
    template <typename... T>
    auto unqualified_assignments_only() const {
        return map_type::decorate(decorators::assignments::unqualified::only<T...>{});
    }
    template <typename... T>
    auto unqualified_assignments_except() const {
        return map_type::decorate(decorators::assignments::unqualified::except<T...>{});
    }
    
    auto conditions() const {
        return map_type::decorate(decorators::conditions{});
    }
    template <typename PrefixT>
    auto conditions(PrefixT prefix) const {
        return map_type::decorate(decorators::conditions::prefixed(std::move(prefix)));
    }
    template <typename... T>
    auto conditions_only() const {
        return map_type::decorate(decorators::conditions::only<T...>{});
    }
    template <typename... T, typename PrefixT>
    auto conditions_only(PrefixT prefix) const {
        return map_type::decorate(decorators::conditions::only<T...>::prefixed(std::move(prefix)));
    }
    template <typename... T>
    auto conditions_except() const {
        return map_type::decorate(decorators::conditions::except<T...>{});
    }
    template <typename... T, typename PrefixT>
    auto conditions_except(PrefixT prefix) const {
        return map_type::decorate(decorators::conditions::except<T...>::prefixed(std::move(prefix)));
    }

    auto definitions() const {
        return map_type::decorate(decorators::definitions{});
    }
    template <typename... T>
    auto definitions_only() const {
        return map_type::decorate(decorators::definitions::only<T...>{});
    }
    template <typename... T>
    auto definitions_except() const {
        return map_type::decorate(decorators::definitions::except<T...>{});
    }
};

template <typename... Fields, typename T>
decltype(auto) operator>>(const basic_schema<Fields...>& sch, T& var){
    const typename basic_schema<Fields...>::map_type& map = static_cast<const typename basic_schema<Fields...>::map_type&>(sch);
    return map.next(var);
}
    
}
}
}

#endif // UDHO_DB_PG_SCHEMA_BASIC_H

