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

#ifndef UDHO_DB_PG_CRUD_UPDATE_H
#define UDHO_DB_PG_CRUD_UPDATE_H

#include <udho/db/pg/activities/activity.h>
#include <udho/db/common/result.h>
#include <udho/db/common/none.h>

namespace udho{
namespace db{
namespace pg{
    
/**
 * update returning one pg::schema<Fields...> (e.g. the record just updated)
 * modifiable pg::schema<Fields...> for assignments using [] 
 * no where query
 */
template <typename SchemaT>
struct basic_update{
    
    using schema_type = SchemaT;
    
    template <typename DerivedT>
    using basic_activity = pg::activity<DerivedT, db::result<schema_type>, schema_type>;
    
    template <typename RelationT>
    struct generators{
        using into_type     = pg::generators::into<RelationT>;
        using set_type      = pg::generators::set<schema_type>;
        
        into_type    into;
        set_type     set;
        
        generators(const schema_type& schema): set(schema){}
        auto operator()() {
            using namespace ozo::literals;
            return "update "_SQL + std::move(into.relation()) + " "_SQL + std::move(set());
        }
    };
    
    template<typename DerivedT>
    struct activity: basic_activity<DerivedT>, schema_type{
        typedef basic_activity<DerivedT> activity_type;
        
        template <typename CollectorT, typename... Args>
        activity(CollectorT collector, pg::connection::pool& pool, boost::asio::io_service& io, Args&&... rest): 
            activity_type(collector, pool, io), 
            schema_type(std::forward<Args>(rest)...)
            {}
        template <typename ContextT, typename... Activities, typename... Args>
        activity(pg::controller<ContextT, Activities...>& ctrl, Args&&... rest): activity(ctrl.data(), ctrl.pool(), ctrl.io(), std::forward<Args>(rest)...){}
        
        schema_type& schema() { return static_cast<schema_type&>(*this); }
        
        using activity_type::operator();
    };
    
    /**
     * update returning one pg::schema<Fields...> (e.g. the record just updated)
     * modifiable pg::schema<Fields...> for assignments using [] 
     * modifiable pg::schema<T...> for where query using with[] 
     */
    template <typename... T>
    struct with{
        
        typedef pg::schema<T...> with_type;
        
        template <typename RelationT>
        struct generators{
            using into_type     = pg::generators::into<RelationT>;
            using set_type      = pg::generators::set<schema_type>;
            using where_type    = pg::generators::where<with_type>;
            
            into_type    into;
            set_type     set;
            where_type   where;
            
            generators(const schema_type& schema, const with_type& with): set(schema), where(with){}
            auto operator()() {
                using namespace ozo::literals;
                return "update "_SQL + std::move(into.relation()) + " "_SQL + std::move(set()) + " "_SQL + std::move(where());
            }
        };
        
        template<typename DerivedT>
        struct activity: basic_activity<DerivedT>, schema_type{
            typedef basic_activity<DerivedT> activity_type;
            
            template <typename CollectorT, typename... Args>
            activity(CollectorT collector, pg::connection::pool& pool, boost::asio::io_service& io, Args&&... rest): 
                activity_type(collector, pool, io), 
                schema_type(std::forward<Args>(rest)...)
                {}
            template <typename ContextT, typename... Activities, typename... Args>
            activity(pg::controller<ContextT, Activities...>& ctrl, Args&&... rest): activity(ctrl.data(), ctrl.pool(), ctrl.io(), std::forward<Args>(rest)...){}
            
            schema_type& schema() { return static_cast<schema_type&>(*this); }
            with_type& with() { return where; }
            
            using activity_type::operator();
            
            template <typename KeyT>
            using schema_has = typename schema_type::types::template has<KeyT>;
            template <typename ElementT>
            using schema_contains = typename schema_type::types::template exists<ElementT>;
            
            template <typename KeyT>
            using with_has = typename with_type::types::template has<KeyT>;
            template <typename ElementT>
            using with_contains = typename with_type::types::template exists<ElementT>;
            
            template <typename KeyT, int = 0, typename = typename std::enable_if<schema_has<KeyT>::value && !schema_contains<KeyT>::value && !with_has<KeyT>::value && !with_contains<KeyT>::value>::type>
            auto& operator[](const KeyT& k){
                return schema_type::template data<KeyT>(k);
            }
            
            template <typename KeyT, int = 0, typename = typename std::enable_if<schema_has<KeyT>::value && !schema_contains<KeyT>::value && !with_has<KeyT>::value && !with_contains<KeyT>::value>::type>
            const auto& operator[](const KeyT& k) const{
                return schema_type::template data<KeyT>(k);
            }
            
            template <typename ElementT, int = 0, typename = typename std::enable_if<!schema_has<ElementT>::value && schema_contains<ElementT>::value && !with_has<ElementT>::value && !with_contains<ElementT>::value>::type>
            auto& operator[](const udho::hazo::element_t<ElementT>& e){
                return schema_type::template element<ElementT>(e);
            }
            
            template <typename ElementT, int = 0, typename = typename std::enable_if<!schema_has<ElementT>::value && schema_contains<ElementT>::value && !with_has<ElementT>::value && !with_contains<ElementT>::value>::type>
            const auto& operator[](const udho::hazo::element_t<ElementT>& e) const{
                return schema_type::template element<ElementT>(e);
            }
            
            template <typename ElementT, typename = typename std::enable_if<!schema_has<ElementT>::value && !schema_contains<ElementT>::value && !with_has<ElementT>::value && with_contains<ElementT>::value>::type>
            auto& operator[](const udho::hazo::element_t<ElementT>& e){
                return where.template element<ElementT>(e);
            }
            
            template <typename ElementT, typename = typename std::enable_if<!schema_has<ElementT>::value && !schema_contains<ElementT>::value && !with_has<ElementT>::value && with_contains<ElementT>::value>::type>
            const auto& operator[](const udho::hazo::element_t<ElementT>& e) const{
                return where.template element<ElementT>(e);
            }
            
            private:
                using schema_type::operator[];
            public:
                with_type where;     // where query
        };
               
        /**
         * update returning one pg::schema<F...> (e.g. the record just updated)
         * modifiable pg::schema<Fields...> for assignments using [] 
         * modifiable pg::schema<T...> for where query using with[] 
         */
        template <typename... F>
        struct returning{
            
            template <typename RelationT>
            struct generators{
                using into_type      = pg::generators::into<RelationT>;
                using set_type       = pg::generators::set<schema_type>;
                using where_type     = pg::generators::where<with_type>;
                using returning_type = pg::generators::returning<pg::schema<F...>>;
                
                into_type      into;
                set_type       set;
                where_type     where;
                returning_type returning;
                
                generators(const schema_type& schema, const with_type& with): set(schema), where(with){}
                auto operator()() {
                    using namespace ozo::literals;
                    return "update "_SQL + std::move(into.relation()) + " "_SQL + std::move(set()) + " "_SQL + std::move(where()) + " "_SQL + std::move(returning());
                }
            };
            
            template<typename DerivedT>
            struct activity: pg::activity<DerivedT, db::result<pg::schema<F...>>>, schema_type{
                typedef pg::activity<DerivedT, db::result<pg::schema<F...>>> activity_type;
                
                template <typename CollectorT, typename... Args>
                activity(CollectorT collector, pg::connection::pool& pool, boost::asio::io_service& io, Args&&... rest): 
                    activity_type(collector, pool, io), 
                    schema_type(std::forward<Args>(rest)...)
                    {}
                template <typename ContextT, typename... Activities, typename... Args>
                activity(pg::controller<ContextT, Activities...>& ctrl, Args&&... rest): activity(ctrl.data(), ctrl.pool(), ctrl.io(), std::forward<Args>(rest)...){}
                
                schema_type& schema() { return static_cast<schema_type&>(*this); }
                with_type& with() { return where; }
                
                using activity_type::operator();
                using schema_type::operator[];
                
                template <typename KeyT>
                using schema_has = typename schema_type::types::template has<KeyT>;
                template <typename ElementT>
                using schema_contains = typename schema_type::types::template exists<ElementT>;
                
                template <typename KeyT>
                using with_has = typename with_type::types::template has<KeyT>;
                template <typename ElementT>
                using with_contains = typename with_type::types::template exists<ElementT>;
                

                template <typename ElementT, typename = typename std::enable_if<!schema_has<ElementT>::value && !schema_contains<ElementT>::value && !with_has<ElementT>::value && with_contains<ElementT>::value>::type>
                ElementT& operator[](const udho::hazo::element_t<ElementT>& e){
                    return where.template element<ElementT>(e);
                }
                
                template <typename ElementT, typename = typename std::enable_if<!schema_has<ElementT>::value && !schema_contains<ElementT>::value && !with_has<ElementT>::value && with_contains<ElementT>::value>::type>
                const ElementT& operator[](const udho::hazo::element_t<ElementT>& e) const{
                    return where.template element<ElementT>(e);
                }
                
                template <typename ElementT, int = 0, typename = typename std::enable_if<!schema_has<ElementT>::value && schema_contains<ElementT>::value && !with_has<ElementT>::value && !with_contains<ElementT>::value>::type>
                ElementT& operator[](const udho::hazo::element_t<ElementT>& e){
                    return schema_type::template element<ElementT>(e);
                }
                
                template <typename ElementT, int = 0, typename = typename std::enable_if<!schema_has<ElementT>::value && schema_contains<ElementT>::value && !with_has<ElementT>::value && !with_contains<ElementT>::value>::type>
                const ElementT& operator[](const udho::hazo::element_t<ElementT>& e) const{
                    return schema_type::template element<ElementT>(e);
                }
                
                public:
                    with_type where;     // where query
            };
            
        };
        
    };
    
    /**
     * update returning one pg::schema<F...> (e.g. the record just updated)
     * modifiable pg::schema<Fields...> for assignments using [] 
     * no where query
     */
    template <typename... F>
    struct returning{
        
            template <typename RelationT>
            struct generators{
                using into_type      = pg::generators::into<RelationT>;
                using set_type       = pg::generators::set<schema_type>;
                using returning_type = pg::generators::returning<pg::schema<F...>>;
                
                into_type      into;
                set_type       set;
                returning_type returning;
                
                generators(const schema_type& schema): set(schema){}
                auto operator()() {
                    using namespace ozo::literals;
                    return "update "_SQL + std::move(into.relation()) + " "_SQL + std::move(set()) + " "_SQL + std::move(returning());
                }
            };
            
            template<typename DerivedT>
            struct activity: pg::activity<DerivedT, db::result<pg::schema<F...>>>, schema_type{
                typedef pg::activity<DerivedT, db::result<pg::schema<F...>>> activity_type;
                
                template <typename CollectorT, typename... Args>
                activity(CollectorT collector, pg::connection::pool& pool, boost::asio::io_service& io, Args&&... rest): 
                    activity_type(collector, pool, io), 
                    schema_type(std::forward<Args>(rest)...)
                    {}
                template <typename ContextT, typename... Activities, typename... Args>
                activity(pg::controller<ContextT, Activities...>& ctrl, Args&&... rest): activity(ctrl.data(), ctrl.pool(), ctrl.io(), std::forward<Args>(rest)...){}
                
                schema_type& schema() { return static_cast<schema_type&>(*this); }
                
                using activity_type::operator();
            };
        
    };
};

template <typename SchemaT>
using update = basic_update<SchemaT>;
    
}
}
}

#endif // UDHO_DB_PG_CRUD_UPDATE_H
