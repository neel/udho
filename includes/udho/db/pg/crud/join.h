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

#ifndef UDHO_DB_PG_CRUD_JOIN_H
#define UDHO_DB_PG_CRUD_JOIN_H

#include <type_traits>
#include <udho/db/pg/crud/fwd.h>
#include <udho/db/pg/pretty.h>
#include <udho/db/pg/crud/many.h>
#include <udho/db/pg/crud/one.h>

namespace udho{
namespace db{
namespace pg{
    
namespace join_types{
    
/**
 * @brief Inner Join
 * 
 */
struct inner{
    static auto keyword(){
        using namespace ozo::literals;
        return "inner join"_SQL;
    }
};

/**
 * @brief Left join
 * 
 */
struct left{
    static auto keyword(){
        using namespace ozo::literals;
        return "left join"_SQL;
    }
};
/**
 * @brief Right join
 * 
 */
struct right{
    static auto keyword(){
        using namespace ozo::literals;
        return "right join"_SQL;
    }
};

/**
 * @brief Full Join
 * 
 */
struct full{
    static auto keyword(){
        using namespace ozo::literals;
        return "full join"_SQL;
    }
};

/**
 * @brief Full Join
 * 
 */
struct full_outer{
    static auto keyword(){
        using namespace ozo::literals;
        return "full outer join"_SQL;
    }
};
    
}
    
/**
 * @brief 
 * 
 * @tparam JoinType Type of join \see join_types::inner,  join_types::outer, join_types::left, join_types::right
 * @tparam RelationL The relation on the left side of join
 * @tparam RelationR The relation on the right side of join
 * @tparam FieldL The field of the left relation 
 * @tparam FieldR The field of the right relation
 */
template <typename JoinType, typename RelationL, typename RelationR, typename FieldL, typename FieldR>
struct joined{
    typedef JoinType  type;
    typedef RelationL relation_left;
    typedef RelationR relation_right;
    typedef FieldL    left;
    typedef FieldR    right;
    typedef typename relation_right::schema schema;
};


template <typename CurrentJoin, typename RestJoin = void>
struct join_clause;

template <typename JoinType, typename RelationL, typename RelationR, typename FieldL, typename FieldR>
struct join_clause<joined<JoinType, RelationL, RelationR, FieldL, FieldR>, void>{
    typedef joined<JoinType, RelationL, RelationR, FieldL, FieldR> head;
    typedef void tail;
    typedef pg::schema<typename RelationL::schema, typename head::schema> schema;
    typedef RelationL source;
};

template <typename JoinType, typename RelationL, typename RelationR, typename FieldL, typename FieldR, typename CurrentJoin, typename RestJoin>
struct join_clause<joined<JoinType, RelationL, RelationR, FieldL, FieldR>, join_clause<CurrentJoin, RestJoin>>{
    typedef joined<JoinType, RelationL, RelationR, FieldL, FieldR> head;
    typedef join_clause<CurrentJoin, RestJoin> tail;
    typedef pg::schema<typename tail::schema, typename head::schema> schema;
    typedef typename tail::source source;
};

template <typename OnlyFieldT, typename SchemaT>
struct column_helper{
    template <typename FieldT>
    using data_of = typename SchemaT::types::template data_of<FieldT>;
    
    template <typename FieldT>
    using relation_of = typename data_of<FieldT>::relation_type;
    
    using column = typename OnlyFieldT::template attach<relation_of>;
};

template <typename... OnlyFields, typename SchemaT>
struct column_helper<pg::basic_schema<OnlyFields...>, SchemaT>{
    using column = pg::schema<OnlyFields...>;
};

/**
 * @brief A node in the chain of multiple joins
 * @internal Not to be used directly, Rather to be used wih the \see basic_join conveniance structure
 * @tparam JoinType Type of join \see join_types::inner,  join_types::outer, join_types::left, join_types::right
 * @tparam FromRelationT The relation on the left side of the join
 * @tparam RelationT The relation on the rigth side of the join
 * @tparam FieldL The field of the left relation
 * @tparam FieldR The field of the right relation
 * @tparam PreviousJoin Previous joins in the chain
 */
template <typename JoinType, typename FromRelationT, typename RelationT, typename FieldL, typename FieldR, typename PreviousJoin>
struct basic_join_on{
    using type = join_clause<joined<JoinType, FromRelationT, RelationT, FieldL, FieldR>, PreviousJoin>;
    using source = typename type::source;
    using schema = typename type::schema;
    
    template <typename OtherRelationT, typename SomeFromT = FromRelationT>
    using join = pg::basic_join<SomeFromT, OtherRelationT, type>;
    
    template <typename FieldT>
    using data_of = typename schema::types::template data_of<FieldT>;
    
    template <typename FieldT>
    using relation_of = typename data_of<FieldT>::relation_type;
    
    static std::string pretty(){
        udho::pretty::printer printer;
        printer.substitute<FieldL>();
        printer.substitute<FieldR>();
        printer.substitute<PreviousJoin>();
        return udho::pretty::demangle<basic_join_on<JoinType, FromRelationT, RelationT, FieldL, FieldR, PreviousJoin>>(printer);
    }
    
    template <typename ResultT>
    struct basic_read{
        using fields = typename pg::builder<basic_join_on<JoinType, FromRelationT, RelationT, FieldL, FieldR, PreviousJoin>>::template select<ResultT>;
        
        template <typename DerivedT>
        using activity = typename fields::template activity<DerivedT>;
        
        using apply = typename fields::apply;
        
        template <typename FieldT>
        using descending = typename fields::template descending<typename FieldT::template attach<relation_of>>;
        template <typename FieldT>
        using ascending  = typename fields::template ascending<typename FieldT::template attach<relation_of>>;
        
        template <int Limit, int Offset = 0>
        using limit = typename fields::template limit<Limit, Offset>;
        
        template <typename... ByFields>
        struct by: fields::template with<typename ByFields::template attach<relation_of>...>{
            using where = typename fields::template with<typename ByFields::template attach<relation_of>...>;
            
            template <typename FieldT>
            using descending = typename where::template descending<typename FieldT::template attach<relation_of>>;
            template <typename FieldT>
            using ascending  = typename where::template ascending<typename FieldT::template attach<relation_of>>;
            
            template <typename... GroupFields>
            struct group_by: where::template group<typename GroupFields::template attach<relation_of>...>{
                using grouped = typename where::template group<typename GroupFields::template attach<relation_of>...>;
                
                template <typename FieldT>
                using descending = typename grouped::template descending<typename FieldT::template attach<relation_of>>;
                template <typename FieldT>
                using ascending  = typename grouped::template ascending<typename FieldT::template attach<relation_of>>;
            };
        };
        
        template <typename... X>
        using exclude = basic_read<typename ResultT::template exclude<typename X::template attach<relation_of>...>>;
        template <typename... X>
        using include = basic_read<typename ResultT::template include<typename X::template attach<relation_of>...>>;
        
        template <typename... GroupFields>
        struct group_by: fields::template group<typename GroupFields::template attach<relation_of>...>{
            using grouped = typename fields::template group<typename GroupFields::template attach<relation_of>...>;
            
            template <typename FieldT>
            using descending = typename grouped::template descending<typename FieldT::template attach<relation_of>>;
            template <typename FieldT>
            using ascending  = typename grouped::template ascending<typename FieldT::template attach<relation_of>>;
        };
    };
    
    struct fetch{
        using all = basic_read<pg::many<schema>>;
        template <typename... OnlyFields>
        using only = basic_read<pg::many<typename column_helper<OnlyFields, schema>::column...>>;
    };
    
    struct retrieve{
        using all = basic_read<pg::one<schema>>;
        template <typename... OnlyFields>
        using only = basic_read<pg::one<typename column_helper<OnlyFields, schema>::column...>>;
    };
};

/**
 * @brief Provides join functionality
 * 
 * @tparam FromRelationT The relation on the left side of the join
 * @tparam RelationT The relation on the right side of the join
 * @tparam PreviousJoin The previous join in the chain or void
 */
template <typename FromRelationT, typename RelationT, typename PreviousJoin>
struct basic_join{
    /**
     * @brief Inner join
     * 
     */
    struct inner{
        /**
         * @brief Specify the columns for inner join
         * 
         * @tparam FieldL The column of the left relation of the join (associated with the FromRelationT)
         * @tparam FieldR The column of the right relation of the join (associated with the RelationT)
         */
        template <typename FieldL, typename FieldR>
        using on = basic_join_on<join_types::inner, FromRelationT, RelationT, FieldL, FieldR, PreviousJoin>;
    };
    /**
     * @brief left join
     * 
     */
    struct left{
        /**
         * @brief Specify the columns for left join
         * 
         * @tparam FieldL The column of the left relation of the join (associated with the FromRelationT)
         * @tparam FieldR The column of the right relation of the join (associated with the RelationT)
         */
        template <typename FieldL, typename FieldR>
        using on = basic_join_on<join_types::left, FromRelationT, RelationT, FieldL, FieldR, PreviousJoin>;
        /**
         * @brief LEFT JOIN is LEFT OUTER JOIN
         * 
         */
        using outer = left;
    };
    /**
     * @brief right join
     * 
     */
    struct right{
        /**
         * @brief Specify the columns for right join
         * 
         * @tparam FieldL The column of the left relation of the join (associated with the FromRelationT)
         * @tparam FieldR The column of the right relation of the join (associated with the RelationT)
         */
        template <typename FieldL, typename FieldR>
        using on = basic_join_on<join_types::right, FromRelationT, RelationT, FieldL, FieldR, PreviousJoin>;
        /**
         * @brief RIGHT JOIN is RIGHT OUTER JOIN
         * 
         */
        using outer = right;
    };
    /**
     * @brief full join
     * 
     */
    struct full{
        /**
         * @brief Specify the columns for full join
         * 
         * @tparam FieldL The column of the left relation of the join (associated with the FromRelationT)
         * @tparam FieldR The column of the right relation of the join (associated with the RelationT)
         */
        template <typename FieldL, typename FieldR>
        using on = basic_join_on<join_types::full, FromRelationT, RelationT, FieldL, FieldR, PreviousJoin>;
        /**
         * @brief full outer join
         * 
         */
        struct outer{
            /**
            * @brief Specify the columns for full outer join
            * 
            * @tparam FieldL The column of the left relation of the join (associated with the FromRelationT)
            * @tparam FieldR The column of the right relation of the join (associated with the RelationT)
            */
            template <typename FieldL, typename FieldR>
            using on = basic_join_on<join_types::full_outer, FromRelationT, RelationT, FieldL, FieldR, PreviousJoin>;
        };
    };
};

/**
 * @brief join attach 
 *
 * @code 
 *   join<post::full>::inner::on<author::id, post::author>
 * ::join<project::full>::outer::on<author::id, project::owner>
 * ::join<group::full>::outer::on<author::id, group::owner>
 * 
 * join_clause<
 *      joined<outer, group::full, author::id, group::owner>,
 *      join_clause<
 *          joined<outer, project::full, author::id, project::owner>,
 *          join_clause<
 *              joined<inner, post::full, author::id, post::author>,
 *              void
 *          >
 *      >
 * >
 * @endcode 
 */
template <typename FromRelationT>
struct attached{
    template <typename RelationT>
    using join = basic_join<FromRelationT, RelationT>;
};
    
}
}
}

#endif // UDHO_DB_PG_CRUD_JOIN_H
