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

#ifndef UDHO_ACTIVITIES_DB_PG_GENERATORS_H
#define UDHO_ACTIVITIES_DB_PG_GENERATORS_H

#include <ozo/query_builder.h>
#include <udho/db/pg/features.h>

namespace udho{
namespace db{
namespace pg{

template <typename FeatureT>
struct feature_generator;
    
template <typename FieldT>
struct feature_generator<pg::extra::ascending<FieldT, true>>{
    auto clause() const {
        using namespace ozo::literals;
        
        return "order by "_SQL + FieldT::ozo_name() + " asc"_SQL;
    }
};

template <typename FieldT>
struct feature_generator<pg::extra::ascending<FieldT, false>>{
    auto clause() const {
        using namespace ozo::literals;
        
        return "order by "_SQL + FieldT::ozo_name() + " desc"_SQL;
    }
};

template <int Limit, int Offset>
struct feature_generator<pg::extra::limited<Limit, Offset>>{
    const pg::extra::limited<Limit, Offset>& _limited;
    
    feature_generator(const pg::extra::limited<Limit, Offset>& limited): _limited(limited){}
    auto clause() const {
        using namespace ozo::literals;
        
        return "limit "_SQL + _limited.limit() + " offset "_SQL + _limited.offset();
    }
};

template <>
struct feature_generator<pg::extra::limited<-1, 0>>{
    const pg::extra::limited<-1, 0>& _limited;
    
    feature_generator(const pg::extra::limited<-1, 0>& limited): _limited(limited){}
    auto clause() const {
        using namespace ozo::literals;
        
        return " limit ALL offset 0"_SQL;
    }
};

template <typename SchemaT, typename OrderingT, typename LimitingT, typename WithT = void>
struct select_generator_ordered_limited{
    typedef SchemaT schema_type;
    typedef WithT with_type;
    typedef feature_generator<OrderingT> ordering_type;
    typedef feature_generator<LimitingT> limiting_type;
    
    const schema_type& schema;
    const with_type& with;
    limiting_type limited;
    ordering_type ordered;
    
    select_generator_ordered_limited(const schema_type& schema_, const LimitingT& limited_, const with_type& with_): schema(schema_), with(with_), limited(limited_){}
    
    template <typename RelationT>
    auto def(RelationT rel){
        return from(rel);
    }
    
    template <typename RelationT>
    auto from(RelationT rel){
        using namespace ozo::literals;
        
        auto name = rel;
        auto prefixed = std::move(rel) + "."_SQL;
        return "select "_SQL + schema.fields(prefixed) 
            + " from "_SQL + std::move(name) 
            + " where "_SQL + with.assignments(prefixed) + " "_SQL + ordered.clause() + " "_SQL + limited.clause();
    }
    
    template <typename... Fields, typename RelationT>
    auto only(RelationT rel){
        using namespace ozo::literals;
        
        auto name = rel;
        auto prefixed = std::move(rel) + "."_SQL;
        return "select "_SQL + schema.template fields_only<Fields...>(prefixed)
            + " from "_SQL + std::move(name)  
            + " where "_SQL + with.assignments(prefixed) + ordered.clause() + " "_SQL + limited.clause();
    }
    template <typename... Fields, typename RelationT>
    auto except(RelationT rel){
        using namespace ozo::literals;
        
        auto name = rel;
        auto prefixed = std::move(rel) + "."_SQL;
        return "select "_SQL + schema.template fields_except<Fields...>(prefixed)
            + " from "_SQL + std::move(name)  
            + " where "_SQL + with.assignments(prefixed) + ordered.clause() + " "_SQL + limited.clause();
    }
};

template <typename SchemaT, typename OrderingT, typename LimitingT>
struct select_generator_ordered_limited<SchemaT, OrderingT, LimitingT>{
    typedef SchemaT schema_type;
    typedef feature_generator<OrderingT> ordering_type;
    typedef feature_generator<LimitingT> limiting_type;
    
    const schema_type& schema;
    ordering_type ordered;
    limiting_type limited;
    
    select_generator_ordered_limited(const schema_type& schema_, const LimitingT& limited_): schema(schema_), limited(limited_) {}
    
    template <typename RelationT>
    auto def(RelationT rel){
        return from(rel);
    }
    
    template <typename RelationT>
    auto from(RelationT rel){
        using namespace ozo::literals;
        
        auto name = rel;
        auto prefixed = std::move(rel) + "."_SQL;
        return "select "_SQL + schema.fields(prefixed) 
            + " from "_SQL + std::move(name) 
            + ordered.clause() + " "_SQL + limited.clause();
    }
    
    template <typename... Fields, typename RelationT>
    auto only(RelationT rel){
        using namespace ozo::literals;
        
        auto name = rel;
        auto prefixed = std::move(rel) + "."_SQL;
        return "select "_SQL + schema.template fields_only<Fields...>(prefixed)
            + " from "_SQL + std::move(name)  
            + ordered.clause() + " "_SQL + limited.clause();
    }
    template <typename... Fields, typename RelationT>
    auto except(RelationT rel){
        using namespace ozo::literals;
        
        auto name = rel;
        auto prefixed = std::move(rel) + "."_SQL;
        return "select "_SQL + schema.template fields_except<Fields...>(prefixed)
            + " from "_SQL + std::move(name)  
            + ordered.clause() + " "_SQL + limited.clause();
    }
};

template <typename SchemaT, typename OrderingT, typename WithT = void>
struct select_generator_ordered{
    typedef SchemaT schema_type;
    typedef WithT with_type;
    typedef feature_generator<OrderingT> ordering_type;
    
    const schema_type& schema;
    const with_type& with;
    ordering_type ordered;
    
    select_generator_ordered(const schema_type& schema_, const with_type& with_): schema(schema_), with(with_){}
    
    template <typename RelationT>
    auto def(RelationT rel){
        return from(rel);
    }
    
    template <typename RelationT>
    auto from(RelationT rel){
        using namespace ozo::literals;
        
        auto name = rel;
        auto prefixed = std::move(rel) + "."_SQL;
        return "select "_SQL + schema.fields(prefixed) 
            + " from "_SQL + std::move(name) 
            + " where "_SQL + with.assignments(prefixed) + ordered.clause();
    }
    
    template <typename... Fields, typename RelationT>
    auto only(RelationT rel){
        using namespace ozo::literals;
        
        auto name = rel;
        auto prefixed = std::move(rel) + "."_SQL;
        return "select "_SQL + schema.template fields_only<Fields...>(prefixed)
            + " from "_SQL + std::move(name)  
            + " where "_SQL + with.assignments(prefixed) + ordered.clause();
    }
    template <typename... Fields, typename RelationT>
    auto except(RelationT rel){
        using namespace ozo::literals;
        
        auto name = rel;
        auto prefixed = std::move(rel) + "."_SQL;
        return "select "_SQL + schema.template fields_except<Fields...>(prefixed)
            + " from "_SQL + std::move(name)  
            + " where "_SQL + with.assignments(prefixed) + ordered.clause();
    }
};

template <typename SchemaT, typename OrderingT>
struct select_generator_ordered<SchemaT, OrderingT, void>{
    typedef SchemaT schema_type;
    typedef feature_generator<OrderingT> ordering_type;
    
    const schema_type& schema;
    ordering_type ordered;
    
    select_generator_ordered(const schema_type& schema_): schema(schema_){}
    
    template <typename RelationT>
    auto def(RelationT rel){
        return from(rel);
    }
    
    template <typename RelationT>
    auto from(RelationT rel){
        using namespace ozo::literals;
        
        auto name = rel;
        auto prefixed = std::move(rel) + "."_SQL;
        return "select "_SQL + schema.fields(prefixed)
            + " from "_SQL + std::move(name) + ordered.clause();
    }
    
    template <typename... Fields, typename RelationT>
    auto only(RelationT rel){
        using namespace ozo::literals;
        
        auto name = rel;
        auto prefixed = std::move(rel) + "."_SQL;
        return "select "_SQL + schema.template fields_only<Fields...>(prefixed)
            + " from "_SQL + std::move(name) + ordered.clause();
    }
    template <typename... Fields, typename RelationT>
    auto except(RelationT rel){
        using namespace ozo::literals;
        
        auto name = rel;
        auto prefixed = std::move(rel) + "."_SQL;
        return "select "_SQL + schema.template fields_except<Fields...>(prefixed)
            + " from "_SQL + std::move(name) + ordered.clause();
    }
};
    
template <typename SchemaT, typename LimitingT, typename WithT = void>
struct select_limited_generator{
    typedef SchemaT schema_type;
    typedef WithT with_type;
    typedef feature_generator<LimitingT> limiting_type;
    
    const schema_type& schema;
    limiting_type limited;
    const with_type& with;
    
    select_limited_generator(const schema_type& schema_, const LimitingT& limited_, const with_type& with_): schema(schema_), limited(limited_), with(with_){}
    
    template <typename RelationT>
    auto def(RelationT rel){
        return from(rel);
    }
    
    template <typename RelationT>
    auto from(RelationT rel){
        using namespace ozo::literals;
        
        auto name = rel;
        auto prefixed = std::move(rel) + "."_SQL;
        return "select "_SQL + schema.fields(prefixed) 
            + " from "_SQL + std::move(name) 
            + " where "_SQL + with.assignments(prefixed)
            + " "_SQL + limited.clause();
    }
    
    template <typename... Fields, typename RelationT>
    auto only(RelationT rel){
        using namespace ozo::literals;
        
        auto name = rel;
        auto prefixed = std::move(rel) + "."_SQL;
        return "select "_SQL + schema.template fields_only<Fields...>(prefixed)
            + " from "_SQL + std::move(name)  
            + " where "_SQL + with.assignments(prefixed)
            + " "_SQL + limited.clause();
    }
    template <typename... Fields, typename RelationT>
    auto except(RelationT rel){
        using namespace ozo::literals;
        
        auto name = rel;
        auto prefixed = std::move(rel) + "."_SQL;
        return "select "_SQL + schema.template fields_except<Fields...>(prefixed)
            + " from "_SQL + std::move(name)  
            + " where "_SQL + with.assignments(prefixed)
            + " "_SQL + limited.clause();
    }
};

template <typename SchemaT, typename LimitingT>
struct select_limited_generator<SchemaT, LimitingT>{
    typedef SchemaT schema_type;
    typedef feature_generator<LimitingT> limiting_type;
    
    const schema_type& schema;
    limiting_type limited;
    
    select_limited_generator(const schema_type& schema_, const LimitingT& limited_): schema(schema_), limited(limited_){}
    
    template <typename RelationT>
    auto def(RelationT rel){
        return from(rel);
    }
    
    template <typename RelationT>
    auto from(RelationT rel){
        using namespace ozo::literals;
        
        auto name = rel;
        auto prefixed = std::move(rel) + "."_SQL;
        return "select "_SQL + schema.fields(prefixed) 
            + " from "_SQL + std::move(name) 
            + " "_SQL + limited.clause();
    }
    
    template <typename... Fields, typename RelationT>
    auto only(RelationT rel){
        using namespace ozo::literals;
        
        auto name = rel;
        auto prefixed = std::move(rel) + "."_SQL;
        return "select "_SQL + schema.template fields_only<Fields...>(prefixed)
            + " from "_SQL + std::move(name)  
            + " "_SQL + limited.clause();
    }
    template <typename... Fields, typename RelationT>
    auto except(RelationT rel){
        using namespace ozo::literals;
        
        auto name = rel;
        auto prefixed = std::move(rel) + "."_SQL;
        return "select "_SQL + schema.template fields_except<Fields...>(prefixed)
            + " from "_SQL + std::move(name)  
            + " "_SQL + limited.clause();
    }
};

template <typename SchemaT, typename WithT = void>
struct select_generator{
    typedef SchemaT schema_type;
    typedef WithT with_type;
    
    const schema_type& schema;
    const with_type& with;
    
    select_generator(const schema_type& schema_, const with_type& with_): schema(schema_), with(with_){}
    
    template <typename RelationT>
    auto def(RelationT rel){
        return from(rel);
    }
    
    template <typename RelationT>
    auto from(RelationT rel){
        using namespace ozo::literals;
        
        auto name = rel;
        auto prefixed = std::move(rel) + "."_SQL;
        return "select "_SQL + schema.fields(prefixed) 
            + " from "_SQL + std::move(name) 
            + " where "_SQL + with.assignments(prefixed);
    }
    
    template <typename... Fields, typename RelationT>
    auto only(RelationT rel){
        using namespace ozo::literals;
        
        auto name = rel;
        auto prefixed = std::move(rel) + "."_SQL;
        return "select "_SQL + schema.template fields_only<Fields...>(prefixed)
            + " from "_SQL + std::move(name)  
            + " where "_SQL + with.assignments(prefixed);
    }
    template <typename... Fields, typename RelationT>
    auto except(RelationT rel){
        using namespace ozo::literals;
        
        auto name = rel;
        auto prefixed = std::move(rel) + "."_SQL;
        return "select "_SQL + schema.template fields_except<Fields...>(prefixed)
            + " from "_SQL + std::move(name)  
            + " where "_SQL + with.assignments(prefixed);
    }
};

template <typename SchemaT>
struct select_generator<SchemaT, void>{
    typedef SchemaT schema_type;
    
    const schema_type& schema;
    
    select_generator(const schema_type& schema_): schema(schema_){}
    
    template <typename RelationT>
    auto def(RelationT rel){
        return from(rel);
    }
    
    template <typename RelationT>
    auto from(RelationT rel){
        using namespace ozo::literals;
        
        auto name = rel;
        auto prefixed = std::move(rel) + "."_SQL;
        return "select "_SQL + schema.fields(prefixed)
            + " from "_SQL + std::move(name);
    }
    
    template <typename... Fields, typename RelationT>
    auto only(RelationT rel){
        using namespace ozo::literals;
        
        auto name = rel;
        auto prefixed = std::move(rel) + "."_SQL;
        return "select "_SQL + schema.template fields_only<Fields...>(prefixed)
            + " from "_SQL + std::move(name);
    }
    template <typename... Fields, typename RelationT>
    auto except(RelationT rel){
        using namespace ozo::literals;
        
        auto name = rel;
        auto prefixed = std::move(rel) + "."_SQL;
        return "select "_SQL + schema.template fields_except<Fields...>(prefixed)
            + " from "_SQL + std::move(name);
    }
};

template <typename AssignmentsSchemaT, typename SchemaT = void>
struct insert_generator{
    typedef AssignmentsSchemaT assignments_type;
    typedef SchemaT schema_type;
    
    const assignments_type& fields;
    const schema_type& schema;
    
    insert_generator(const assignments_type& with_, const schema_type& schema_): fields(with_), schema(schema_){}
    
    template <typename RelationT>
    auto def(RelationT rel){
        return into(rel);
    }
    
    template <typename RelationT>
    auto into(RelationT rel){
        using namespace ozo::literals;
        
        return "insert into "_SQL + std::move(rel) 
            + "("_SQL + fields.fields() + ")"_SQL
            + " values "_SQL
            + "("_SQL + fields.values() + ")"_SQL
            + " returning "_SQL + schema.fields();
    }
    
    template <typename... Fields, typename RelationT>
    auto only(RelationT rel){
        using namespace ozo::literals;
        
        return "insert into "_SQL + std::move(rel) 
            + "("_SQL + fields.template fields_only<Fields...>() + ")"_SQL
            + " values "_SQL
            + "("_SQL + fields.template values_only<Fields...>() + ")"_SQL
            + " returning "_SQL + schema.fields();
    }
    template <typename... Fields, typename RelationT>
    auto except(RelationT rel){
        using namespace ozo::literals;
        
        return "insert into "_SQL + std::move(rel) 
            + "("_SQL + fields.template fields_except<Fields...>() + ")"_SQL
            + " values "_SQL
            + "("_SQL + fields.template values_except<Fields...>() + ")"_SQL
            + " returning "_SQL + schema.fields();
    }
};

template <typename AssignmentsSchemaT>
struct insert_generator<AssignmentsSchemaT, void>{
    typedef AssignmentsSchemaT assignments_type;
    
    const assignments_type& fields;
    
    insert_generator(const assignments_type& with_): fields(with_){}
    
    template <typename RelationT>
    auto def(RelationT rel){
        return into(rel);
    }
    
    template <typename RelationT>
    auto into(RelationT rel){
        using namespace ozo::literals;
        
        return "insert into "_SQL + std::move(rel) 
            + "("_SQL + fields.fields() + ")"_SQL
            + " values "_SQL
            + "("_SQL + fields.values() + ")"_SQL;
    }
    
    template <typename... Fields, typename RelationT>
    auto only(RelationT rel){
        using namespace ozo::literals;
        
        return "insert into "_SQL + std::move(rel) 
            + "("_SQL + fields.template fields_only<Fields...>() + ")"_SQL
            + " values "_SQL
            + "("_SQL + fields.template values_only<Fields...>() + ")"_SQL;
    }
    template <typename... Fields, typename RelationT>
    auto except(RelationT rel){
        using namespace ozo::literals;
        
        return "insert into "_SQL + std::move(rel) 
            + "("_SQL + fields.template fields_except<Fields...>() + ")"_SQL
            + " values "_SQL
            + "("_SQL + fields.template values_except<Fields...>() + ")"_SQL;
    }
};

template <typename AssignmentsSchemaT, typename SchemaT, typename WithT = void>
struct returning_update_generator{
    typedef AssignmentsSchemaT assignments_type;
    typedef SchemaT schema_type;
    typedef WithT with_type;
    
    const assignments_type& fields;
    const schema_type& schema;
    const with_type& with;
    
    returning_update_generator(const assignments_type& assignments_, const schema_type& schema_, const with_type& with_): fields(assignments_), schema(schema_), with(with_){}
    
    template <typename RelationT>
    auto def(RelationT rel){
        return into(rel);
    }
    
    template <typename RelationT>
    auto into(RelationT rel){
        using namespace ozo::literals;
        
        return "update "_SQL + std::move(rel) + " set "_SQL
            + fields.assignments()
            + " where "_SQL + with.assignments()
            + " returning "_SQL+ schema.fields();
    }
    
    template <typename... Fields, typename RelationT>
    auto only(RelationT rel){
        using namespace ozo::literals;
        
        return "update "_SQL + std::move(rel) + " set "_SQL
            + fields.template assignments_only<Fields...>()
            + " where "_SQL + with.assignments()
            + " returning "_SQL+ schema.fields();
    }
    
    template <typename... Fields, typename RelationT>
    auto except(RelationT rel){
        using namespace ozo::literals;
        
        return "update "_SQL + std::move(rel) + " set "_SQL
            + fields.template assignments_except<Fields...>()
            + " where "_SQL + with.assignments()
            + " returning "_SQL+ schema.fields();
    }
};

template <typename AssignmentsSchemaT, typename SchemaT>
struct returning_update_generator<AssignmentsSchemaT, SchemaT, void>{
    typedef AssignmentsSchemaT assignments_type;
    typedef SchemaT schema_type;
    
    const assignments_type& fields;
    const schema_type& schema;
    
    returning_update_generator(const assignments_type& assignments_, const schema_type& schema_): fields(assignments_), schema(schema_) {}
    
    template <typename RelationT>
    auto def(RelationT rel){
        return into(rel);
    }
    
    template <typename RelationT>
    auto into(RelationT rel){
        using namespace ozo::literals;
        
        return "update "_SQL + std::move(rel) 
            + " set "_SQL + fields.assignments()
            + " returning "_SQL + schema.fields();
    }
    
    template <typename... Fields, typename RelationT>
    auto only(RelationT rel){
        using namespace ozo::literals;
        
        return "update "_SQL + std::move(rel) 
            + " set "_SQL + fields.template assignments_only<Fields...>()
            + " returning "_SQL + schema.fields();
    }
    
    template <typename... Fields, typename RelationT>
    auto except(RelationT rel){
        using namespace ozo::literals;
        
        return "update "_SQL + std::move(rel) 
            + " set "_SQL + fields.template assignments_except<Fields...>()
            + " returning "_SQL + schema.fields();
    }
};

template <typename AssignmentsSchemaT, typename WithT = void>
struct update_generator{
    typedef AssignmentsSchemaT assignments_type;
    typedef WithT with_type;
    
    const assignments_type& fields;
    const with_type& with;
    
    update_generator(const assignments_type& assignments_, const with_type& with_): fields(assignments_), with(with_){}
    
    template <typename RelationT>
    auto def(RelationT rel){
        return into(rel);
    }
    
    template <typename RelationT>
    auto into(RelationT rel){
        using namespace ozo::literals;
        
        return "update "_SQL + std::move(rel) + " set "_SQL
            + fields.assignments()
            + " where "_SQL + with.assignments();
    }
    
    template <typename... Fields, typename RelationT>
    auto only(RelationT rel){
        using namespace ozo::literals;
        
        return "update "_SQL + std::move(rel) + " set "_SQL
            + fields.template assignments_only<Fields...>()
            + " where "_SQL + with.assignments();
    }
    
    template <typename... Fields, typename RelationT>
    auto except(RelationT rel){
        using namespace ozo::literals;
        
        return "update "_SQL + std::move(rel) + " set "_SQL
            + fields.template assignments_except<Fields...>()
            + " where "_SQL + with.assignments();
    }
};

template <typename AssignmentsSchemaT>
struct update_generator<AssignmentsSchemaT, void>{
    typedef AssignmentsSchemaT assignments_type;
    
    const assignments_type& fields;
    
    update_generator(const assignments_type& assignments_): fields(assignments_){}
    
    template <typename RelationT>
    auto def(RelationT rel){
        return into(rel);
    }
    
    template <typename RelationT>
    auto into(RelationT rel){
        using namespace ozo::literals;
        
        return "update "_SQL + std::move(rel) + " set "_SQL
            + fields.assignments();
    }
    
    template <typename... Fields, typename RelationT>
    auto only(RelationT rel){
        using namespace ozo::literals;
        
        return "update "_SQL + std::move(rel) + " set "_SQL
            + fields.template assignments_only<Fields...>();
    }
    
    template <typename... Fields, typename RelationT>
    auto except(RelationT rel){
        using namespace ozo::literals;
        
        return "update "_SQL + std::move(rel) + " set "_SQL
            + fields.template assignments_except<Fields...>();
    }
};

template <typename WithT>
struct delete_generator{
    typedef WithT with_type;
    
    const with_type& with;
    
    delete_generator(const with_type& with_): with(with_){}
    
    template <typename RelationT>
    auto def(RelationT rel){
        return from(rel);
    }
    
    template <typename RelationT>
    auto from(RelationT rel){
        using namespace ozo::literals;
        
        return "delete from "_SQL + std::move(rel) 
            + " where "_SQL + with.assignments();
    }
};

}
}
}

#endif // UDHO_ACTIVITIES_DB_PG_GENERATORS_H
