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

#ifndef UDHO_DB_PG_CRUD_SELECT_H
#define UDHO_DB_PG_CRUD_SELECT_H

#include <udho/db/pg/schema/schema.h>
#include <udho/db/pg/activities/activity.h>
#include <udho/db/pg/generators/fwd.h>
#include <udho/db/pg/crud/fwd.h>
#include <udho/db/pg/crud/limit.h>
#include <udho/db/pg/crud/order.h>

namespace udho{
namespace db{
namespace pg{
    
/**
 * @brief basic select query
 * @ingroup crud
 * @tparam ResultT 
 * @tparam SchemaT 
 */
template <typename ResultT, typename SchemaT>
struct basic_select{
    using schema_type = SchemaT;
    template <typename DerivedT>
    using basic_activity = pg::activity<DerivedT, ResultT, schema_type>;
    
    basic_select() = delete;
    ~basic_select() = delete;
    basic_select(const basic_select<ResultT, SchemaT>&) = delete;
    basic_select<ResultT, SchemaT>& operator=(const basic_select<ResultT, SchemaT>&) = delete;
    
    /**
     * @brief basic select query generator, only consists of select and from part.
     * @tparam RelationT 
     */
    template <typename RelationT>
    struct generators{
        using select_type = pg::generators::select<schema_type>;
        using from_type   = pg::generators::from<RelationT>;
        
        /**
         * @brief generate select part of the SQL query
         */
        select_type select;
        /**
         * @brief generate from part of the SQL query
         */
        from_type   from;
        
        generators(const schema_type& schema): select(schema){}

        /**
         * @brief generate the default SELECT Fields... FROM ... query.
         * @return auto 
         */
        auto operator()() {
            using namespace ozo::literals;
            return std::move(select()) + " "_SQL + std::move(from());
        }
    };
    
    /**
     * @brief pg activity may be subclassed if there is a requirements for custom SQL queries. 
     * The DerivedT must provide operator() overload that sends the custom SQL query to the query() function.
     * @tparam DerivedT 
     */
    template<typename DerivedT>
    struct activity: basic_activity<DerivedT>{
        typedef basic_activity<DerivedT> activity_type;

        // typedef generators<RelationT> generators;
        
        // generators generate;
        
        template <typename CollectorT>
        activity(CollectorT collector, pg::connection::pool& pool, boost::asio::io_service& io): activity_type(collector, pool, io) /*, generate(activity_type::result)*/ {}
        template <typename ContextT, typename... T>
        activity(pg::controller<ContextT, T...>& ctrl): activity(ctrl.data(), ctrl.pool(), ctrl.io()){}
        
        using activity_type::operator();
        
        protected:
            schema_type result;
    };
       
    /**
     * @brief Add limit clause the the SQL query
     * 
     * @tparam Limit 
     * @tparam Offset 
     */
    template <int Limit, int Offset = 0>
    struct limit{
        typedef pg::limited<Limit, Offset> limited_type;
        
        limit() = delete;
        ~limit() = delete;
        limit(const limit<Limit, Offset>&) = delete;
        limit<Limit, Offset>& operator=(const limit<Limit, Offset>&) = delete;
        
        /**
         * @brief select query generator, consists of select, from and limit clause.
         * @tparam RelationT 
         */
        template <typename RelationT>
        struct generators{
            using select_type = pg::generators::select<schema_type>;
            using from_type   = pg::generators::from<RelationT>;
            using limit_type  = pg::generators::limit<limited_type>;
            
            /**
             * @brief generate select clause of the SQL query
             */
            select_type select;
            /**
             * @brief generate from clause of the SQL query
             */
            from_type   from;
            /**
             * @brief generate limit clause of the SQL query
             * 
             */
            limit_type  limit;
            
            generators(const schema_type& schema, const limit_type& limited): select(schema), limit(limited){}

            /**
             * @brief generate the default SELECT Fields... FROM ... LIMIT <Limit> OFFSET <Offset> query.
             * @return auto 
             */
            auto operator()() {
                using namespace ozo::literals;
                return std::move(select()) + " "_SQL + std::move(from()) + " "_SQL + std::move(limit());
            }
        };
        
        /**
         * @brief pg activity may be subclassed if there is a requirements for custom SQL queries. 
         * The DerivedT must provide operator() overload that sends the custom SQL query to the query() function.
         * @tparam DerivedT 
         */
        template <typename DerivedT>
        struct activity: basic_activity<DerivedT>, limited_type{
            typedef basic_activity<DerivedT> activity_type;

            // typedef generators<RelationT> generators;
            
            // generators generate;
            
            template <typename CollectorT>
            activity(CollectorT collector, pg::connection::pool& pool, boost::asio::io_service& io): activity_type(collector, pool, io)/*, generate(activity_type::result)*/ {}
            template <typename ContextT, typename... T>
            activity(pg::controller<ContextT, T...>& ctrl): activity(ctrl.data(), ctrl.pool(), ctrl.io()){}
            
            limited_type& limited() { return static_cast<limited_type&>(*this); }
            
            using activity_type::operator();
                
            protected:
                schema_type result;
        };

    };
    
    template <typename OrderedFieldT>
    struct ordered{
        ordered() = delete;
        ~ordered() = delete;
        ordered(const ordered<OrderedFieldT>&) = delete;
        ordered<OrderedFieldT>& operator=(const ordered<OrderedFieldT>&) = delete;
        
        template <typename RelationT>
        struct generators{
            using select_type = pg::generators::select<schema_type>;
            using from_type   = pg::generators::from<RelationT>;
            using order_type  = pg::generators::order<OrderedFieldT>;
            
            select_type select;
            from_type   from;
            order_type  order;
            
            generators(const schema_type& schema): select(schema){}
            auto operator()() {
                using namespace ozo::literals;
                return std::move(select()) + " "_SQL + std::move(from()) + " "_SQL + std::move(order());
            }
        };
        
        template <typename DerivedT>
        struct activity: basic_activity<DerivedT>{
            typedef basic_activity<DerivedT> activity_type;

            // typedef generators<RelationT> generators;
            
            // generators generate;
            
            template <typename CollectorT>
            activity(CollectorT collector, pg::connection::pool& pool, boost::asio::io_service& io): activity_type(collector, pool, io)/*, generate(activity_type::result)*/ {}
            template <typename ContextT, typename... T>
            activity(pg::controller<ContextT, T...>& ctrl): activity(ctrl.data(), ctrl.pool(), ctrl.io()){}
            
            using activity_type::operator();
            
            protected:
                schema_type result;
        };
        
        template <int Limit, int Offset = 0>
        struct limit{
            typedef pg::limited<Limit, Offset> limited_type;
            
            template <typename RelationT>
            struct generators{
                using select_type = pg::generators::select<schema_type>;
                using from_type   = pg::generators::from<RelationT>;
                using limit_type  = pg::generators::limit<limited_type>;
                using order_type  = pg::generators::order<OrderedFieldT>;
                
                select_type select;
                from_type   from;
                order_type  order;
                limit_type  limit;
                
                generators(const schema_type& schema, const limit_type& limited): select(schema), limit(limited){}
                auto operator()() {
                    using namespace ozo::literals;
                    return std::move(select()) + " "_SQL + std::move(from()) + " "_SQL + std::move(order()) + " "_SQL + std::move(limit());
                }
            };
            
            template <typename DerivedT>
            struct activity: basic_activity<DerivedT>, limited_type{
                typedef basic_activity<DerivedT> activity_type;

                // typedef generators<RelationT> generators;
                
                // generators generate;
                
                template <typename CollectorT>
                activity(CollectorT collector, pg::connection::pool& pool, boost::asio::io_service& io): activity_type(collector, pool, io)/*, generate(activity_type::result, activity_type::limited())*/ {}
                template <typename ContextT, typename... T>
                activity(pg::controller<ContextT, T...>& ctrl): activity(ctrl.data(), ctrl.pool(), ctrl.io())/*, generate(activity_type::result, activity_type::limited())*/{}
                    
                using activity_type::operator();
                
                limited_type& limited() { return static_cast<limited_type&>(*this); }
                
                protected:
                    schema_type result;
            };
            
        };
    };
    
    template <typename FieldT>
    using descending = ordered<pg::descending<FieldT>>;
    template <typename FieldT>
    using ascending = ordered<pg::ascending<FieldT>>;
    
    template <typename... Fields>
    struct with{
        
        typedef pg::schema<Fields...> with_type;
        
        with() = delete;
        ~with() = delete;
        with(const with<Fields...>&) = delete;
        with<Fields...>& operator=(const with<Fields...>&) = delete;
        
        template <typename RelationT>
        struct generators{
            using select_type = pg::generators::select<schema_type>;
            using from_type   = pg::generators::from<RelationT>;
            using where_type  = pg::generators::where<with_type>;
            using values_type = pg::generators::values<with_type>;
            
            select_type select;
            from_type   from;
            where_type  where;
            values_type values;
            
            generators(const schema_type& schema, const with_type& with): select(schema), where(with), values(with){}
            auto operator()() {
                using namespace ozo::literals;
                return std::move(select()) + " "_SQL + std::move(from()) + " "_SQL + std::move(where());
            }
        };
        
        template <typename DerivedT>
        struct activity: basic_activity<DerivedT>, with_type{
            typedef basic_activity<DerivedT> activity_type;

            // typedef generators<RelationT> generators;
            
            // generators generate;
            
            template <typename CollectorT, typename... Args>
            activity(CollectorT collector, pg::connection::pool& pool, boost::asio::io_service& io, const Args&... args): activity_type(collector, pool, io), with_type(args...)/*, generate(activity_type::result, activity_type::with())*/ {}
            template <typename ContextT, typename... T, typename... Args>
            activity(pg::controller<ContextT, T...>& ctrl, const Args&... args): activity(ctrl.data(), ctrl.pool(), ctrl.io(), args...)/*, generate(activity_type::result, activity_type::with())*/{}
            
            with_type& with() { return static_cast<with_type&>(*this); }
            
            using activity_type::operator();
            
            protected:
                schema_type result;
        };
        
        template <int Limit, int Offset = 0>
        struct limit{
            typedef pg::limited<Limit, Offset> limited_type;
            
            limit() = delete;
            ~limit() = delete;
            limit(const limit<Limit, Offset>&) = delete;
            limit<Limit, Offset>& operator=(const limit<Limit, Offset>&) = delete;
            
            template <typename RelationT>
            struct generators{
                using select_type = pg::generators::select<schema_type>;
                using from_type   = pg::generators::from<RelationT>;
                using where_type  = pg::generators::where<with_type>;
                using limit_type  = pg::generators::limit<limited_type>;
                
                select_type select;
                from_type   from;
                where_type  where;
                limit_type  limit;
                
                generators(const schema_type& schema, const with_type& with, const limit_type& limited): select(schema), where(with), limit(limited){}
                auto operator()() {
                    using namespace ozo::literals;
                    return std::move(select()) + " "_SQL + std::move(from()) + " "_SQL + std::move(where()) + " "_SQL + std::move(limit());
                }
            };
            
            template <typename DerivedT>
            struct activity: basic_activity<DerivedT>, with_type, limited_type{
                typedef basic_activity<DerivedT> activity_type;

                // typedef generators<RelationT> generators;
                
                // generators generate;
                
                template <typename CollectorT, typename... Args>
                activity(CollectorT collector, pg::connection::pool& pool, boost::asio::io_service& io, const Args&... args): activity_type(collector, pool, io), with_type(args...)/*, generate(activity_type::result, activity_type::with(), activity_type::limited())*/ {}
                template <typename ContextT, typename... T, typename... Args>
                activity(pg::controller<ContextT, T...>& ctrl, const Args&... args): activity(ctrl.data(), ctrl.pool(), ctrl.io(), args...)/*, generate(activity_type::result, activity_type::with(), activity_type::limited())*/{}
                
                with_type& with() { return static_cast<with_type&>(*this); }
                limited_type& limited() { return static_cast<limited_type&>(*this); }
                
                using activity_type::operator();
                
                protected:
                    schema_type result;
            };

        };
        
        template <typename OrderedFieldT>
        struct ordered{
            
            ordered() = delete;
            ~ordered() = delete;
            ordered(const ordered<OrderedFieldT>&) = delete;
            ordered<OrderedFieldT>& operator=(const ordered<OrderedFieldT>&) = delete;
            
            template <typename RelationT>
            struct generators{
                using select_type = pg::generators::select<schema_type>;
                using from_type   = pg::generators::from<RelationT>;
                using order_type  = pg::generators::order<OrderedFieldT>;
                using where_type  = pg::generators::where<with_type>;
                
                select_type select;
                from_type   from;
                order_type  order;
                where_type  where;
                
                generators(const schema_type& schema, const with_type& with): select(schema), where(with){}
                auto operator()() {
                    using namespace ozo::literals;
                    return std::move(select()) + " "_SQL + std::move(from()) + " "_SQL + std::move(where()) + " "_SQL + std::move(order());
                }
            };
            
            template <typename DerivedT>
            struct activity: basic_activity<DerivedT>, with_type{
                typedef basic_activity<DerivedT> activity_type;

                // typedef generators<RelationT> generators;
                
                // generators generate;
                
                template <typename CollectorT, typename... Args>
                activity(CollectorT collector, pg::connection::pool& pool, boost::asio::io_service& io, const Args&... args): activity_type(collector, pool, io), with_type(args...) /*, generate(activity_type::result, activity_type::with())*/ {}
                template <typename ContextT, typename... T, typename... Args>
                activity(pg::controller<ContextT, T...>& ctrl, const Args&... args): activity(ctrl.data(), ctrl.pool(), ctrl.io(), args...) /*, generate(activity_type::result, activity_type::with())*/{}
                    
                using activity_type::operator();
                
                with_type& with() { return static_cast<with_type&>(*this); }
                
                protected:
                    schema_type result;
            };
            
            template <int Limit, int Offset = 0>
            struct limit{
                typedef pg::limited<Limit, Offset> limited_type;
                
                limit() = delete;
                ~limit() = delete;
                limit(const limit<Limit, Offset>&) = delete;
                limit<Limit, Offset>& operator=(const limit<Limit, Offset>&) = delete;
                
                template <typename RelationT>
                struct generators{
                    using select_type = pg::generators::select<schema_type>;
                    using from_type   = pg::generators::from<RelationT>;
                    using where_type  = pg::generators::where<with_type>;
                    using order_type  = pg::generators::order<OrderedFieldT>;
                    using limit_type  = pg::generators::limit<limited_type>;
                    
                    select_type select;
                    from_type   from;
                    where_type  where;
                    order_type  order;
                    limit_type  limit;
                    
                    generators(const schema_type& schema, const with_type& with, const limit_type& limited): select(schema), where(with), limit(limited){}
                    auto operator()() {
                        using namespace ozo::literals;
                        return std::move(select()) + " "_SQL + std::move(from()) + " "_SQL + std::move(where()) + " "_SQL + std::move(order()) + " "_SQL + std::move(limit());
                    }
                };
                
                template <typename DerivedT>
                struct activity: basic_activity<DerivedT>, with_type, limited_type{
                    typedef basic_activity<DerivedT> activity_type;

                    // typedef generators<RelationT> generators;
                    
                    // generators generate;
                    
                    template <typename CollectorT, typename... Args>
                    activity(CollectorT collector, pg::connection::pool& pool, boost::asio::io_service& io, const Args&... args): activity_type(collector, pool, io), with_type(args...) /*, generate(activity_type::result, activity_type::with(), activity_type::limited())*/ {}
                    template <typename ContextT, typename... T, typename... Args>
                    activity(pg::controller<ContextT, T...>& ctrl, const Args&... args): activity(ctrl.data(), ctrl.pool(), ctrl.io(), args...)/*, generate(activity_type::result, activity_type::with(), activity_type::limited())*/{}
                        
                    using activity_type::operator();
                    
                    with_type& with() { return static_cast<with_type&>(*this); }
                    limited_type& limited() { return static_cast<limited_type&>(*this); }
                    
                    protected:
                        schema_type result;
                }; 

            };
 
        };
        
        template <typename FieldT>
        using descending = ordered<pg::descending<FieldT>>;
        template <typename FieldT>
        using ascending = ordered<pg::ascending<FieldT>>;
        
        template <typename... GroupColumn>
        struct group{
            group() = delete;
            ~group() = delete;
            group(const group<GroupColumn...>&) = delete;
            group<GroupColumn...>& operator=(const group<GroupColumn...>&) = delete;
            
            template <typename RelationT>
            struct generators{
                using select_type = pg::generators::select<schema_type>;
                using from_type   = pg::generators::from<RelationT>;
                using where_type  = pg::generators::where<with_type>;
                using group_type  = pg::generators::group<GroupColumn...>;
                
                select_type select;
                from_type   from;
                where_type  where;
                group_type  group;
                
                generators(const schema_type& schema, const with_type& with): select(schema), where(with){}
                auto operator()() {
                    using namespace ozo::literals;
                    return std::move(select()) + " "_SQL + std::move(from()) + " "_SQL + std::move(where()) + " "_SQL + std::move(group());
                }
            };
            
            template <typename DerivedT>
            struct activity: basic_activity<DerivedT>, with_type{
                typedef basic_activity<DerivedT> activity_type;

                // typedef generators<RelationT> generators;
                
                // generators generate;
                
                template <typename CollectorT, typename... Args>
                activity(CollectorT collector, pg::connection::pool& pool, boost::asio::io_service& io, const Args&... args): activity_type(collector, pool, io), with_type(args...)/*, generate(activity_type::result, activity_type::with())*/ {}
                template <typename ContextT, typename... T, typename... Args>
                activity(pg::controller<ContextT, T...>& ctrl, const Args&... args): activity(ctrl.data(), ctrl.pool(), ctrl.io(), args...)/*, generate(activity_type::result, activity_type::with())*/{}
                
                with_type& with() { return static_cast<with_type&>(*this); }
                
                using activity_type::operator();
                
                protected:
                    schema_type result;
            };
            
            template <typename OrderedFieldT>
            struct ordered{
                
                ordered() = delete;
                ~ordered() = delete;
                ordered(const ordered<OrderedFieldT>&) = delete;
                ordered<OrderedFieldT>& operator=(const ordered<OrderedFieldT>&) = delete;
                
                template <typename RelationT>
                struct generators{
                    using select_type = pg::generators::select<schema_type>;
                    using from_type   = pg::generators::from<RelationT>;
                    using where_type  = pg::generators::where<with_type>;
                    using group_type  = pg::generators::group<GroupColumn...>;
                    using order_type  = pg::generators::order<OrderedFieldT>;
                    
                    select_type select;
                    from_type   from;
                    where_type  where;
                    group_type  group;
                    order_type  order;
                    
                    generators(const schema_type& schema, const with_type& with): select(schema), where(with){}
                    auto operator()() {
                        using namespace ozo::literals;
                        return std::move(select()) + " "_SQL + std::move(from()) + " "_SQL + std::move(where()) + " "_SQL + std::move(group())+ " "_SQL + std::move(order());
                    }
                };
                
                template <typename DerivedT>
                struct activity: basic_activity<DerivedT>, with_type{
                    typedef basic_activity<DerivedT> activity_type;

                    // typedef generators<RelationT> generators;
                    
                    // generators generate;
                    
                    template <typename CollectorT, typename... Args>
                    activity(CollectorT collector, pg::connection::pool& pool, boost::asio::io_service& io, const Args&... args): activity_type(collector, pool, io), with_type(args...)/*, generate(activity_type::result, activity_type::with())*/ {}
                    template <typename ContextT, typename... T, typename... Args>
                    activity(pg::controller<ContextT, T...>& ctrl, const Args&... args): activity(ctrl.data(), ctrl.pool(), ctrl.io(), args...)/*, generate(activity_type::result, activity_type::with())*/{}
                        
                    using activity_type::operator();
                    
                    with_type& with() { return static_cast<with_type&>(*this); }
                    
                    protected:
                        schema_type result;
                };
                
                template <int Limit, int Offset = 0>
                struct limit{
                    typedef pg::limited<Limit, Offset> limited_type;
                    
                    limit() = delete;
                    ~limit() = delete;
                    limit(const limit<Limit, Offset>&) = delete;
                    limit<Limit, Offset>& operator=(const limit<Limit, Offset>&) = delete;
                    
                    template <typename RelationT>
                    struct generators{
                        using select_type = pg::generators::select<schema_type>;
                        using from_type   = pg::generators::from<RelationT>;
                        using where_type  = pg::generators::where<with_type>;
                        using group_type  = pg::generators::group<GroupColumn...>;
                        using order_type  = pg::generators::order<OrderedFieldT>;
                        using limit_type  = pg::generators::limit<limited_type>;
                        
                        select_type select;
                        from_type   from;
                        where_type  where;
                        group_type  group;
                        order_type  order;
                        limit_type  limit;
                        
                        generators(const schema_type& schema, const with_type& with, const limit_type& limited): select(schema), where(with), limit(limited){}
                        auto operator()() {
                            using namespace ozo::literals;
                            return std::move(select()) + " "_SQL + std::move(from()) + " "_SQL + std::move(where()) + " "_SQL + std::move(group())+ " "_SQL + std::move(order()) + " "_SQL + std::move(limit());
                        }
                    };
                    
                    template <typename DerivedT>
                    struct activity: basic_activity<DerivedT>, with_type, limited_type{
                        typedef basic_activity<DerivedT> activity_type;

                        // typedef generators<RelationT> generators;
                        
                        // generators generate;
                        
                        template <typename CollectorT, typename... Args>
                        activity(CollectorT collector, pg::connection::pool& pool, boost::asio::io_service& io, const Args&... args): activity_type(collector, pool, io), with_type(args...) /*, generate(activity_type::result, activity_type::with(), activity_type::limited())*/ {}
                        template <typename ContextT, typename... T, typename... Args>
                        activity(pg::controller<ContextT, T...>& ctrl, const Args&... args): activity(ctrl.data(), ctrl.pool(), ctrl.io(), args...)/*, generate(activity_type::result, activity_type::with(), activity_type::limited())*/{}
                            
                        using activity_type::operator();
                        
                        with_type& with() { return static_cast<with_type&>(*this); }
                        limited_type& limited() { return static_cast<limited_type&>(*this); }
                        
                        protected:
                            schema_type result;
                    }; 

                };
    
            };
        };
    };
       
    template <typename... GroupColumn>
    struct group{
        group() = delete;
        ~group() = delete;
        group(const group<GroupColumn...>&) = delete;
        group<GroupColumn...>& operator=(const group<GroupColumn...>&) = delete;
        
        template <typename RelationT>
        struct generators{
            using select_type = pg::generators::select<schema_type>;
            using from_type   = pg::generators::from<RelationT>;
            using group_type  = pg::generators::group<GroupColumn...>;
            
            select_type select;
            from_type   from;
            group_type  group;
            
            generators(const schema_type& schema): select(schema) {}
            auto operator()() {
                using namespace ozo::literals;
                return std::move(select()) + " "_SQL + std::move(from()) + " "_SQL + std::move(group());
            }
        };
        
        template <typename DerivedT>
        struct activity: basic_activity<DerivedT>{
            typedef basic_activity<DerivedT> activity_type;

            // typedef generators<RelationT> generators;
            
            // generators generate;
            
            template <typename CollectorT, typename... Args>
            activity(CollectorT collector, pg::connection::pool& pool, boost::asio::io_service& io): activity_type(collector, pool, io)/*, generate(activity_type::result) */{}
            template <typename ContextT, typename... T, typename... Args>
            activity(pg::controller<ContextT, T...>& ctrl): activity(ctrl.data(), ctrl.pool(), ctrl.io())/*, generate(activity_type::result) */{}
            
            using activity_type::operator();
            
            protected:
                schema_type result;
        };
        

        template <typename OrderedFieldT>
        struct ordered{
            
            ordered() = delete;
            ~ordered() = delete;
            ordered(const ordered<OrderedFieldT>&) = delete;
            ordered<OrderedFieldT>& operator=(const ordered<OrderedFieldT>&) = delete;
            
            template <typename RelationT>
            struct generators{
                using select_type = pg::generators::select<schema_type>;
                using from_type   = pg::generators::from<RelationT>;
                using group_type  = pg::generators::group<GroupColumn...>;
                using order_type  = pg::generators::order<OrderedFieldT>;
                
                select_type select;
                from_type   from;
                group_type  group;
                order_type  order;
                
                generators(const schema_type& schema): select(schema){}
                auto operator()() {
                    using namespace ozo::literals;
                    return std::move(select()) + " "_SQL + std::move(from()) + " "_SQL + std::move(group())+ " "_SQL + std::move(order());
                }
            };
            
            template <typename DerivedT>
            struct activity: basic_activity<DerivedT>{
                typedef basic_activity<DerivedT> activity_type;

                // typedef generators<RelationT> generators;
                
                // generators generate;
                
                template <typename CollectorT, typename... Args>
                activity(CollectorT collector, pg::connection::pool& pool, boost::asio::io_service& io): activity_type(collector, pool, io)/*, generate(activity_type::result) */ {}
                template <typename ContextT, typename... T, typename... Args>
                activity(pg::controller<ContextT, T...>& ctrl): activity(ctrl.data(), ctrl.pool(), ctrl.io())/*, generate(activity_type::result) */{}
                    
                using activity_type::operator();
                
                protected:
                    schema_type result;
            };
            
            template <int Limit, int Offset = 0>
            struct limit{
                typedef pg::limited<Limit, Offset> limited_type;
                
                limit() = delete;
                ~limit() = delete;
                limit(const limit<Limit, Offset>&) = delete;
                limit<Limit, Offset>& operator=(const limit<Limit, Offset>&) = delete;
                
                template <typename RelationT>
                struct generators{
                    using select_type = pg::generators::select<schema_type>;
                    using from_type   = pg::generators::from<RelationT>;
                    using group_type  = pg::generators::group<GroupColumn...>;
                    using order_type  = pg::generators::order<OrderedFieldT>;
                    using limit_type  = pg::generators::limit<limited_type>;
                    
                    select_type select;
                    from_type   from;
                    group_type  group;
                    order_type  order;
                    limit_type  limit;
                    
                    generators(const schema_type& schema, const limit_type& limited): select(schema), limit(limited){}
                    auto operator()() {
                        using namespace ozo::literals;
                        return std::move(select()) + " "_SQL + std::move(from()) + " "_SQL + std::move(group())+ " "_SQL + std::move(order()) + " "_SQL + std::move(limit());
                    }
                };
                
                template <typename DerivedT>
                struct activity: basic_activity<DerivedT>, limited_type{
                    typedef basic_activity<DerivedT> activity_type;

                    // typedef generators<RelationT> generators;
                    
                    // generators generate;
                    
                    template <typename CollectorT, typename... Args>
                    activity(CollectorT collector, pg::connection::pool& pool, boost::asio::io_service& io): activity_type(collector, pool, io)/*, generate(activity_type::result, activity_type::limited()) */ {}
                    template <typename ContextT, typename... T, typename... Args>
                    activity(pg::controller<ContextT, T...>& ctrl): activity(ctrl.data(), ctrl.pool(), ctrl.io())/*, generate(activity_type::result, activity_type::limited()) */{}
                        
                    using activity_type::operator();
                    
                    limited_type& limited() { return static_cast<limited_type&>(*this); }
                    
                    protected:
                        schema_type result;
                }; 

            };

        };
    };
};

/**
 * select<pg::many<Fields...>> or select<pg::one<Fields...>>
 * \code
 * struct students_by_grade: 
 *   select<pg::many<student::id, student::name, student::grade, student::marks>>::into<StudentRecord>
 *   ::by<student::grade>
 *   ::ordered<descending<student::marks>>
 *   ::limit<10>::activity
 *  {
 *      using limit::limit;
 *      using limit::operator();
 *      void operator()(){
 *          using namespace ozo::literals;
 *          query(generate.from("students"_SQL));
 *      }
 *  };
 * \endcode
 */
template <typename SuccessT>
struct select: basic_select<typename SuccessT::result_type, typename SuccessT::schema_type>{
    template <typename RecordT>
    using into = basic_select<typename SuccessT::template record_type<RecordT>, typename SuccessT::schema_type>;
};

}
}
}

#endif // UDHO_DB_PG_CRUD_SELECT_H
