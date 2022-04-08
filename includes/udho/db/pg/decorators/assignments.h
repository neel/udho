/*
 * Copyright (c) 2020, <copyright holder> <email>
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
 * THIS SOFTWARE IS PROVIDED BY <copyright holder> <email> ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <copyright holder> <email> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef UDHO_DB_PG_DECORATORS_ASSIGNMENTS_H
#define UDHO_DB_PG_DECORATORS_ASSIGNMENTS_H

#include <utility>
#include <type_traits>
#include <ozo/query_builder.h>
#include <udho/db/pg/decorators/logical.h>
#include <udho/db/pg/decorators/comma.h>
#include <udho/db/pg/decorators/traits.h>
#include <udho/db/pg/schema/column.h>

namespace udho{
namespace db{
namespace pg{

namespace decorators{

/**
 * @ingroup decorators
 * @addtogroup assignments
 * @brief decorates a schema as comma separeted assignments
 * Given the following:
 * @code 
 * namespace students{
 *   PG_ELEMENT(id,    std::int64_t);
 *   PG_ELEMENT(name,  std::string);
 *   PG_ELEMENT(grade, std::int64_t);
 *   PG_ELEMENT(marks, std::int64_t);
 *   
 *   struct table: pg::relation<table, id, name, grade, marks>{
 *       static auto name(){
 *           return "students"_SQL;
 *       }
 *   };
 * }
 * @endcode 
 * Different assignment decorators produces different outputs when applied on the schema. The decorate
 * method returns an OZO string that has `text()` and `params()` method. The `text()` method produces 
 * `boost::hana::string` with placeholders and the `params()` produces a tuple with values. Following
 * are the examples of applying the decorators in the schema mentioned above.
 * @code 
 * student.decorate(pg::decorators::assignments{});
 * // .text():   students.id = $1, students.name = $2, students.grade = $3, students.marks = $4
 * // .params(): boost::hana::tuple<std::int64_t, std::string, std::int64_t, std::int64_t>(42, "Neel Basu", 1, 100)
 * student.decorate(pg::decorators::assignments::unqualified{});
 * // .text():   id = $1, name = $2, grade = $3, marks = $4
 * // .params(): boost::hana::tuple<std::int64_t, std::string, std::int64_t, std::int64_t>(42, "Neel Basu", 1, 100)
 * student.decorate(pg::decorators::assignments::prefixed("s"_SQL));
 * // .text():   s.id = $1, s.name = $2, s.grade = $3, s.marks = $4
 * // .params(): boost::hana::tuple<std::int64_t, std::string, std::int64_t, std::int64_t>(42, "Neel Basu", 1, 100)
 * student.decorate(pg::decorators::assignments::only<students::id, students::name, students::marks>{});
 * // .text():   students.id = $1, students.name = $2, students.marks = $3
 * // .params(): boost::hana::tuple<std::int64_t, std::string, std::int64_t>(42, "Neel Basu", 100)
 * student.decorate(pg::decorators::assignments::only<students::id, students::name, students::marks>::unqualified{});
 * // .text():   id = $1, name = $2, marks = $3
 * // .params(): boost::hana::tuple<std::int64_t, std::string, std::int64_t>(42, "Neel Basu", 100)
 * student.decorate(pg::decorators::assignments::only<students::id, students::name, students::marks>::prefixed("s"_SQL));
 * // .text():   s.id = $1, s.name = $2, s.marks = $3
 * // .params(): boost::hana::tuple<std::int64_t, std::string, std::int64_t>(42, "Neel Basu", 100)
 * student.decorate(pg::decorators::assignments::except<students::grade>{});
 * // .text():   students.id = $1, students.name = $2, students.marks = $3
 * // .params(): boost::hana::tuple<std::int64_t, std::string, std::int64_t>(42, "Neel Basu", 100)
 * student.decorate(pg::decorators::assignments::except<students::grade>::unqualified{});
 * // .text():   id = $1, name = $2, marks = $3
 * // .params(): boost::hana::tuple<std::int64_t, std::string, std::int64_t>(42, "Neel Basu", 100)
 * student.decorate(pg::decorators::assignments::except<students::grade>::prefixed("s"_SQL));
 * // .text():   s.id = $1, s.name = $2, s.marks = $3
 * // .params(): boost::hana::tuple<std::int64_t, std::string, std::int64_t>(42, "Neel Basu", 100)
 * @endcode 
 * @{
 */


/**
 * @brief Decorate the schema as comma separeted assignments while using the given field trait.
 * @tparam FieldTraitT The field decoration @ref traits "trait"
 */
template <typename FieldTraitT>
struct basic_assignments: private FieldTraitT{
    template <typename... Args>
    basic_assignments(Args&&... args): FieldTraitT(std::forward<Args>(args)...){}
    
    template <typename FieldT>
    decltype(auto) operator()(const FieldT& f){
        using namespace ozo::literals;
        using nullable = std::optional<typename FieldT::value_type>;
        return FieldTraitT::apply(f) + " = "_SQL + std::move(f.null() ? nullable() : nullable(f.value()));
    }
    template <typename FieldT, typename ResultT>
    decltype(auto) operator()(const FieldT& f, ResultT res){
        using namespace ozo::literals;
        return pg::decorators::comma(""_SQL, std::move(operator()(f)), std::move(res));
    }
    template <typename ResultT>
    auto finish(ResultT res){
        return res;
    }
};

/**
 * @brief Decorate a subset of the schema as comma separeted assignments while using the given field trait.
 * @tparam FieldTraitT The field decoration @ref traits "trait"
 * @tparam Enabler To decide whether to include a given field or not. Usually one of the following:
 *         - @ref udho::db::pg::decorators::logical_and "logical_and" 
 *         - @ref udho::db::pg::decorators::logical_or "logical_or"
 *         - @ref udho::db::pg::decorators::logical_or "logical_nor"
 *         - @ref udho::db::pg::decorators::logical_or "logical_nand"
 *         .
 * @tparam Fields... The subset of fields of the schema.
 */
template <typename FieldTraitT, template <bool...> class Enabler, typename... Fields>
struct selected_assignments: private FieldTraitT{
    template <typename SubjectT>
    using enable  = typename std::enable_if<Enabler<pg::is_equivalent<SubjectT, Fields>::value...>::value>;
    template <typename SubjectT>
    using disable = typename std::enable_if<!Enabler<pg::is_equivalent<SubjectT, Fields>::value...>::value>;
    
    template <typename... Args>
    selected_assignments(Args&&... args): FieldTraitT(std::forward<Args>(args)...){}
    
    template <typename FieldT, typename = typename enable<FieldT>::type>
    auto operator()(const FieldT& f){
        using namespace ozo::literals;
        using nullable = std::optional<typename FieldT::value_type>;
        return FieldTraitT::apply(f) + " = "_SQL + std::move(f.null() ? nullable() : nullable(f.value()));
    }
    template <typename FieldT, typename ResultT, typename = typename enable<FieldT>::type>
    auto operator()(const FieldT& f, ResultT res){
        using namespace ozo::literals;
        return pg::decorators::comma(""_SQL, std::move(operator()(f)), std::move(res));
    }
    template <typename FieldT, int = 0, typename = typename disable<FieldT>::type>
    decltype(auto) operator()(const FieldT& /*f*/){
        using namespace ozo::literals;
        return ""_SQL;
    }
    template <typename FieldT, typename ResultT, int = 0, typename = typename disable<FieldT>::type>
    decltype(auto) operator()(const FieldT& /*f*/, ResultT res){
        return res;
    }
    template <typename ResultT>
    auto finish(ResultT res){
        return res;
    }
};
    
/**
 * @brief Decorate the schema as assignment, using unqualified field names, while considering only the fields provided.
 * @tparam Fields... The subset of fields of the schema.
 */
template <typename... Fields>
using assignments_only_unqualified = selected_assignments<traits::fields::unqualified, logical_or, Fields...>;

/**
 * @brief Similar to @ref assignments_only_unqualified while using prefixed trait for fields
 * @tparam PrefixT Compile time string to be prefixed with each field e.g. an alias of a relation
 * @tparam Fields... The subset of fields of the schema.
 */
template <typename PrefixT, typename... Fields>
using assignments_only_prefixed = selected_assignments<traits::fields::prefixed<PrefixT>, logical_or, Fields...>;

/**
 * @brief Decorate the schema as assignment, while considering only the fields provided.
 * @tparam Fields... The subset of fields of the schema.
 */
template <typename... Fields>
struct assignments_only: selected_assignments<traits::fields::transparent, logical_or, Fields...>{
    template <typename PrefixT>
    static constexpr assignments_only_prefixed<PrefixT, Fields...> prefixed(PrefixT&& k) {
        return assignments_only_prefixed<PrefixT, Fields...>(std::forward<PrefixT>(k));
    }
    using unqualified = assignments_only_unqualified<Fields...>;
};

/**
 * @brief Decorate the schema as assignment, using unqualified field names, while excluding the fields specified.
 * @tparam Fields... The subset of fields of the schema.
 */
template <typename... Fields>
using assignments_except_unqualified = selected_assignments<traits::fields::unqualified, logical_nor, Fields...>;

/**
 * @brief Similar to @ref assignments_except_unqualified while using prefixed trait for fields
 * @tparam PrefixT Compile time string to be prefixed with each field e.g. an alias of a relation
 * @tparam Fields... The subset of fields of the schema.
 */
template <typename PrefixT, typename... Fields>
using assignments_except_prefixed = selected_assignments<traits::fields::prefixed<PrefixT>, logical_nor, Fields...>;

/**
 * @brief Decorate the schema as assignment, while excluding the fields specified.
 * @tparam Fields... The subset of fields of the schema.
 */
template <typename... Fields>
struct assignments_except: selected_assignments<traits::fields::transparent, logical_nor, Fields...>{
    template <typename PrefixT>
    static constexpr assignments_except_prefixed<PrefixT, Fields...> prefixed(PrefixT&& k) {
        return assignments_except_prefixed<PrefixT, Fields...>(std::forward<PrefixT>(k));
    }
    using unqualified = assignments_except_unqualified<Fields...>;
};

/**
 * @brief Decorate the schema as assignment, using unqualified field trait, while using all fields in the schema.
 */
struct assignments_unqualified: basic_assignments<traits::fields::unqualified>{
    template <typename... Fields>
    using only = assignments_only_unqualified<Fields...>;
    template <typename... Fields>
    using except = assignments_except_unqualified<Fields...>;
};
/**
 * @brief Similar to @ref assignments_unqualified while using prefixed trait for fields
 * @tparam PrefixT Compile time string to be prefixed with each field e.g. an alias of a relation
 */
template <typename PrefixT>
using assignments_prefixed = basic_assignments<traits::fields::prefixed<PrefixT>>;

/**
 * @brief Decorate the schema as assignment, while using all fields in the schema.
 */
struct assignments: basic_assignments<traits::fields::transparent>{
    /**
     * @brief include only a subset of fields
     * @tparam Fields... Subset of fields to include
     */
    template <typename... Fields>
    using only = assignments_only<Fields...>;
    /**
     * @brief include only a subset of fields
     * @tparam Fields... Subset of fields to exclude
     */
    template <typename... Fields>
    using except = assignments_except<Fields...>;
    
    /**
     * @brief Use a the provided string as a prefix.
     * 
     * @tparam PrefixT Compile time OZO string
     * @param k Compile time OZO string
     * @return constexpr assignments_prefixed<PrefixT> 
     */
    template <typename PrefixT>
    static constexpr assignments_prefixed<PrefixT> prefixed(PrefixT&& k) {
        return assignments_prefixed<PrefixT>(std::forward<PrefixT>(k));
    }

    /**
     * @brief force the field names to be unqualified even if the fields are columns.
     */
    using unqualified = assignments_unqualified;
};

/**
 * @)
 */

}

}
}
}

#endif // UDHO_DB_PG_DECORATORS_ASSIGNMENTS_H
