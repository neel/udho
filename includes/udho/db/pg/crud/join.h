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
    
/**
 * @brief Different types of join
 * @ingroup crud
 */
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
 * @brief Datastructure to store a pair of relations and the corresponding fields to be joined at compile time
 * @note Not to be used directly. Used internally by @ref basic_join_on
 * @ingroup crud
 * @tparam JoinType Type of join @ref join_types
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

/**
 * @brief Compile time datastructure to store a chain of chaining multiple joins
 * @note Not to be used directly. Used internally by @ref basic_join_on
 * @ingroup crud
 * @tparam CurrentJoin A join defined by the @ref joined template
 * @tparam RestJoin An optional join clause
 */
template <typename CurrentJoin, typename RestJoin = void>
struct join_clause;

/**
 * @brief Specialization for Simple join clause.
 * @note Not to be used directly. Used internally by @ref basic_join_on
 * @ingroup crud
 * @tparam JoinType Type of join @ref join_types
 * @tparam RelationL The relation on the left side of join
 * @tparam RelationR The relation on the right side of join
 * @tparam FieldL The field of the left relation 
 * @tparam FieldR The field of the right relation
 */
template <typename JoinType, typename RelationL, typename RelationR, typename FieldL, typename FieldR>
struct join_clause<joined<JoinType, RelationL, RelationR, FieldL, FieldR>, void>{
    typedef joined<JoinType, RelationL, RelationR, FieldL, FieldR> head; // TODO maybe unused
    typedef void tail;
    /**
     * @brief Produces a schema by merging the schema of both RelationL and RelationR
     */
    typedef pg::schema<typename RelationL::schema, typename RelationR::schema> schema;
    // typedef pg::schema<typename RelationL::schema, typename head::schema> schema;
    /**
     * @brief Source relation (RelationL)
     */
    typedef RelationL source;
};

/**
 * @brief Specialization for a join clause chained with another.
 * @note Not to be used directly. Used internally by @ref basic_join_on
 * @ingroup crud
 * @tparam JoinType Type of join @ref join_types
 * @tparam RelationL The relation on the left side of join
 * @tparam RelationR The relation on the right side of join
 * @tparam FieldL The field of the left relation 
 * @tparam FieldR The field of the right relation
 * @tparam CurrentJoin 
 * @tparam RestJoin 
 */
template <typename JoinType, typename RelationL, typename RelationR, typename FieldL, typename FieldR, typename CurrentJoin, typename RestJoin>
struct join_clause<joined<JoinType, RelationL, RelationR, FieldL, FieldR>, join_clause<CurrentJoin, RestJoin>>{
    typedef joined<JoinType, RelationL, RelationR, FieldL, FieldR> head; // TODO maybe unused
    typedef join_clause<CurrentJoin, RestJoin> tail;
    /**
     * @brief Produces a schema by merging the schema RelationR with the schema of the other join_clause
     */
    typedef pg::schema<typename tail::schema, typename RelationR::schema> schema;
    // typedef pg::schema<typename tail::schema, typename head::schema> schema;
    /**
     * @brief Source relation of the tail
     */
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
 * @brief A node in the chain of multiple joins.
 * Datastructure to define a join of two columns accross two tables or to define a chain of multiple such joins.
 * @tparam JoinType Type of join \ref join_types::inner,  join_types::outer, join_types::left, join_types::right
 * @tparam FromRelationT The relation on the left side of the join
 * @tparam RelationT The relation on the right side of the join
 * @tparam FieldL The field of the left relation
 * @tparam FieldR The field of the right relation
 * @tparam PreviousJoin Previous joins in the chain
 * 
 * @note Not to be used directly, Rather to be used wih the \ref basic_join convenience structure
 * @ingroup crud
 * The `FromRelationT` is associated with `FieldL` and used as the lhs column for composing JOIN SQL queries. Similarly 
 * the `RelationT` is associated with `FieldR` and used as the  while composing the JOIN SQL query. Chain of multiple
 * JOINs can be constructed using the `PreviousJoin` template parameter which is `void` by default. `JoinType` is used
 * to specify the type of join, e.g. left, tight, inner, outer etc..
 * @code 
 * pg::basic_join_on<
 *    pg::join_types::inner,  // INNER JOIN
 *    articles::table,        // FROM table
 *    students::table,        // JOIN table 
 *    articles::author,       // JOIN lhs
 *    students::id,           // JOIN rhs
 *    void                    // No previous joins
 * >::fetch::all::apply;
 * @endcode 
 * The above generate SQL like the following
 * @code 
 * select                              
 *     articles.id,                    
 *     articles.title,                 
 *     articles.author,                
 *     articles.project,               
 *     students.id,                    
 *     students.name,                  
 *     students.project,               
 *     students.marks                  
 * from articles                       
 * inner join students                 
 *     on articles.author = students.id
 * @endcode 
 * For multiple joins, FROM is choosen from the inner most `PreviousJoin` in the above mentioned way. In the example shown
 * below two tables `projects` and `articles` are joined with the same table (which is in FROM) using different foreign keys.
 * @code 
 * pg::basic_join_on<
 *     pg::join_types::inner,
 *     articles::table,                // Joined with the FROM table
 *     students::table,                // JOIN table (2)
 *     articles::author,               // JOIN lhs   (2)
 *     students::id,                   // JOIN rhs   (2)
 *     pg::basic_join_on<
 *         pg::join_types::inner,
 *         articles::table,            // FROM table
 *         projects::table,            // JOIN table (1)
 *         articles::project,          // JOIN lhs   (1)
 *         projects::id,               // JOIN rhs   (1)
 *         void
 *     >::type
 * >::fetch::all::apply;
 * @endcode 
 * The above may generate SQL like the following 
 * @code 
 * select                                
 *     articles.id,                      
 *     articles.title,                   
 *     articles.author,                  
 *     articles.project,                 
 *     projects.id,                      
 *     projects.title,                   
 *     projects.admin,                   
 *     students.id,                      
 *     students.name,                    
 *     students.project,                 
 *     students.marks                    
 * from articles                            // FROM table
 * inner join projects                      // JOIN table (1)
 *     on articles.project = projects.id    // JOIN       (1)
 * inner join students                      // JOIN table (2)
 *     on articles.author = students.id     // JOIN       (2)
 * @endcode 
 * In the following example, the second join refers to a field from the first join table.
 * @code 
 * pg::basic_join_on<
 *     pg::join_types::inner,
 *     projects::table,                // Joined with the JOIN (1) table
 *     students::table,                // JOIN table (2)
 *     projects::admin,                // JOIN lhs   (2)
 *     students::id,                   // JOIN rhs   (2)
 *     pg::basic_join_on<
 *         pg::join_types::inner,
 *         articles::table,            // FROM table
 *         projects::table,            // JOIN table (1)
 *         articles::project,          // JOIN lhs   (1)
 *         projects::id,               // JOIN rhs   (1)
 *         void
 *     >::type
 * >::fetch::all::apply;
 * @endcode 
 * The above code can be used to generate the following SQL.
 * @code 
 * select                                
 *     articles.id,                      
 *     articles.title,                   
 *     articles.author,                  
 *     articles.project,                 
 *     projects.id,                      
 *     projects.title,                   
 *     projects.admin,                   
 *     students.id,                      
 *     students.name,                    
 *     students.project,                 
 *     students.marks                    
 * from articles                            // FROM table     
 * inner join projects                      // JOIN table (1)
 *     on articles.project = projects.id    // JOIN       (1)
 * inner join students                      // JOIN table (2)
 *     on projects.admin = students.id      // JOIN       (2)
 * @endcode 
 * @warning The `FromRelationT` must either be the FROM table or a table present in the `PreviousJoin` at some depth.
 *          @code 
 *          pg::basic_join_on<
 *              pg::join_types::inner,
 *              students::table,                // Joined with a table which is neither in FROM not in Previous JOIN
 *              projects::table,                // JOIN table (2)
 *              students::id,                   // JOIN lhs   (2)
 *              projects::admin,                // JOIN rhs   (2)
 *              pg::basic_join_on<
 *                  pg::join_types::inner,
 *                  articles::table,            // FROM table
 *                  projects::table,            // JOIN table (1)
 *                  articles::project,          // JOIN lhs   (1)
 *                  projects::id,               // JOIN rhs   (1)
 *                  void
 *              >::type
 *          >::fetch::all::apply;
 *          @endcode 
 *          the above code produces a MALFORMED SQL as shown below.
 *          @code 
 *          select                               
 *              articles.id,                     
 *              articles.title,                  
 *              articles.author,                 
 *              articles.project,                
 *              projects.id,                     
 *              projects.title,                  
 *              projects.admin,                  
 *              projects.id,                     
 *              projects.title,                  
 *              projects.admin                   
 *          from articles                        
 *          inner join projects                  
 *              on articles.project = projects.id
 *          inner join projects                  
 *              on students.id = projects.admin  
 *          @endcode 
 * 
 * Not limited to two, but there can be any number of joins chained together as shown in the example below.
 * @code 
 * pg::basic_join_on<
 *     pg::join_types::inner,
 *     memberships::table,
 *     projects::table,
 *     memberships::project,
 *     projects::id,
 *     pg::basic_join_on<
 *         pg::join_types::inner,
 *         students::table,
 *         memberships::table,
 *         students::id,
 *         memberships::student,
 *         pg::basic_join_on<
 *             pg::join_types::inner,
 *             articles::table,
 *             students::table,
 *             articles::author,
 *             students::id,
 *             void
 *         >::type
 *     >::type
 * >::fetch::all::apply;
 * @endcode 
 * The above will produce SQL like the following.
 * @code 
 * select                                   
 *     articles.id,                         
 *     articles.title,                      
 *     articles.author,                     
 *     articles.project,                    
 *     students.id,                         
 *     students.name,                       
 *     students.project,                    
 *     students.marks,                      
 *     memberships.id,                      
 *     memberships.student,                 
 *     memberships.project,                 
 *     projects.id,                         
 *     projects.title,                      
 *     projects.admin                       
 * from articles                            
 * inner join students                      
 *     on articles.author = students.id     
 * inner join memberships                   
 *     on students.id = memberships.student 
 * inner join projects                      
 *     on memberships.project = projects.id 
 * @endcode 
 */
template <typename JoinType, typename FromRelationT, typename RelationT, typename FieldL, typename FieldR, typename PreviousJoin>
struct basic_join_on{
    /**
     * @brief Corresponding JOIN clause
     */
    using type = join_clause<joined<JoinType, FromRelationT, RelationT, FieldL, FieldR>, PreviousJoin>;
    /**
     * @brief The source relation (used as FROM)
     */
    using source = typename type::source;
    /**
     * @brief The schema consists of all relations in the from and join clauses
     */
    using schema = typename type::schema;
    
    /**
     * @brief Join some other relation
     * 
     * @tparam OtherRelationT 
     * @tparam SomeFromT 
     */
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
 * @ingroup crud
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
 * @ingroup crud
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
