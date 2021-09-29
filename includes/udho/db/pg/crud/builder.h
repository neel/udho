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

#ifndef UDHO_DB_PG_CRUD_BUILDER_H
#define UDHO_DB_PG_CRUD_BUILDER_H

#include <ozo/query_builder.h>
#include <udho/db/pg/crud/select.h>
#include <udho/db/pg/crud/update.h>
#include <udho/db/pg/crud/insert.h>
#include <udho/db/pg/crud/remove.h>
#include <udho/db/pg/crud/limit.h>
#include <udho/db/pg/crud/order.h>
#include <udho/db/pg/pretty.h>

namespace udho{
namespace db{
namespace pg{
    
template <typename RelationT>
struct builder{
    template <typename SuccessT>
    struct select{
        typedef typename pg::select<SuccessT> select_;
        
        template <typename DerivedT>
        struct activity: select_::template activity<DerivedT>{
            typedef typename select_::template activity<DerivedT> base;
            typedef typename select_::template generators<RelationT> generators;
            
            generators generate;
            
            template <typename CollectorT>
            activity(CollectorT collector, pg::connection::pool& pool, boost::asio::io_service& io): base(collector, pool, io), generate(base::result) {}
            template <typename ContextT, typename... T>
            activity(pg::controller<ContextT, T...>& ctrl): base(ctrl), generate(base::result){}
            
            using base::operator();
        };
        
        struct apply: activity<apply>{
            using activity<apply>::activity;
            using activity<apply>::operator();
            
            auto sql(){ return std::move(activity<apply>::generate()); }
            void operator()(){ activity<apply>::query(sql()); }
            static std::string pretty(){
                udho::pretty::printer printer;
                printer.substitute<RelationT>();
                printer.substitute<SuccessT>();
                return udho::pretty::demangle<apply>(printer);
            }
        };
        
        template <int Limit, int Offset = 0>
        struct limit{
            typedef typename select_::template limit<Limit, Offset> limit_;
            
            template <typename DerivedT>
            struct activity: limit_::template activity<DerivedT>{
                typedef typename limit_::template activity<DerivedT> limit_activity;
                typedef typename limit_::template generators<RelationT> generators;
                
                generators generate;
                
                template <typename CollectorT>
                activity(CollectorT collector, pg::connection::pool& pool, boost::asio::io_service& io): limit_activity(collector, pool, io), generate(limit_activity::result, limit_activity::limited()) {}
                template <typename ContextT, typename... T>
                activity(pg::controller<ContextT, T...>& ctrl): limit_activity(ctrl), generate(limit_activity::result, limit_activity::limited()){}
                
                using limit_activity::operator();
            };
            
            struct apply: activity<apply>{
                using activity<apply>::activity;
                using activity<apply>::operator();
                
                auto sql(){ return std::move(activity<apply>::generate()); }
                void operator()(){ activity<apply>::query(sql()); }
                static std::string pretty(){
                    udho::pretty::printer printer;
                    printer.substitute<RelationT>();
                    printer.substitute<SuccessT>();
                    return udho::pretty::demangle<apply>(printer);
                }
            };
        };
        
        template <typename OrderedFieldT>
        struct ordered{
            typedef typename select_::template ordered<OrderedFieldT> ordered_;
            
            template <typename DerivedT>
            struct activity: ordered_::template activity<DerivedT>{
                typedef typename ordered_::template activity<DerivedT> base;
                typedef typename ordered_::template generators<RelationT> generators;
                
                generators generate;
                
                template <typename CollectorT>
                activity(CollectorT collector, pg::connection::pool& pool, boost::asio::io_service& io): base(collector, pool, io), generate(base::result) {}
                template <typename ContextT, typename... T>
                activity(pg::controller<ContextT, T...>& ctrl): base(ctrl), generate(base::result){}
                
                using base::operator();
            };
            
            struct apply: activity<apply>{
                using activity<apply>::activity;
                using activity<apply>::operator();
                
                auto sql(){ return std::move(activity<apply>::generate()); }
                void operator()(){ activity<apply>::query(sql()); }
                static std::string pretty(){
                    udho::pretty::printer printer;
                    printer.substitute<RelationT>();
                    printer.substitute<SuccessT>();
                    printer.substitute<OrderedFieldT>();
                    return udho::pretty::demangle<apply>(printer);
                }
            };
            
            template <int Limit, int Offset = 0>
            struct limit{
                typedef typename ordered_::template limit<Limit, Offset> limit_;
                
                template <typename DerivedT>
                struct activity: limit_::template activity<DerivedT>{
                    typedef typename limit_::template activity<DerivedT> limit_activity;
                    typedef typename limit_::template generators<RelationT> generators;
                    
                    generators generate;
                    
                    template <typename CollectorT>
                    activity(CollectorT collector, pg::connection::pool& pool, boost::asio::io_service& io): limit_activity(collector, pool, io), generate(limit_activity::result, limit_activity::limited()) {}
                    template <typename ContextT, typename... T>
                    activity(pg::controller<ContextT, T...>& ctrl): limit_activity(ctrl), generate(limit_activity::result, limit_activity::limited()){}
                    
                    using limit_activity::operator();
                };
                
                struct apply: activity<apply>{
                    using activity<apply>::activity;
                    using activity<apply>::operator();
                    
                    auto sql(){ return std::move(activity<apply>::generate()); }
                    void operator()(){ activity<apply>::query(sql()); }
                    static std::string pretty(){
                        udho::pretty::printer printer;
                        printer.substitute<RelationT>();
                        printer.substitute<SuccessT>();
                        printer.substitute<OrderedFieldT>();
                        return udho::pretty::demangle<apply>(printer);
                    }
                };
            };
        };
        
        template <typename FieldT>
        using descending = ordered<pg::descending<FieldT>>;
        template <typename FieldT>
        using ascending = ordered<pg::ascending<FieldT>>;
        
        template <typename... GroupColumn>
        struct group{
            typedef typename select_::template group<GroupColumn...> group_;
            
            template <typename DerivedT>
            struct activity: group_::template activity<DerivedT>{
                typedef typename group_::template activity<DerivedT> group_activity;
                typedef typename group_::template generators<RelationT> generators;
                
                generators generate;
                
                template <typename CollectorT, typename... Args>
                activity(CollectorT collector, pg::connection::pool& pool, boost::asio::io_service& io, const Args&... args): group_activity(collector, pool, io, args...), generate(group_activity::result) {}
                template <typename ContextT, typename... T, typename... Args>
                activity(pg::controller<ContextT, T...>& ctrl, const Args&... args): group_activity(ctrl, args...), generate(group_activity::result, group_activity::with()){}
                
                using group_activity::operator();
            };
            
            struct apply: activity<apply>{
                using activity<apply>::activity;
                using activity<apply>::operator();
                
                auto sql(){ return std::move(activity<apply>::generate()); }
                void operator()(){ activity<apply>::query(sql()); }
                static std::string pretty(){
                    udho::pretty::printer printer;
                    printer.substitute<RelationT>();
                    printer.substitute<SuccessT>();
                    printer.substitute_all<GroupColumn...>();
                    return udho::pretty::demangle<apply>(printer);
                }
            };
            
            template <typename OrderedFieldT>
            struct ordered{
                typedef typename group_::template ordered<OrderedFieldT> ordered_;
                
                template <typename DerivedT>
                struct activity: ordered_::template activity<DerivedT>{
                    typedef typename ordered_::template activity<DerivedT> base;
                    typedef typename ordered_::template generators<RelationT> generators;

                    generators generate;
                    
                    template <typename CollectorT, typename... Args>
                    activity(CollectorT collector, pg::connection::pool& pool, boost::asio::io_service& io, const Args&... args): base(collector, pool, io, args...), generate(base::result, base::with()) {}
                    template <typename ContextT, typename... T, typename... Args>
                    activity(pg::controller<ContextT, T...>& ctrl, const Args&... args): base(ctrl, args...), generate(base::result, base::with()){}
                    
                    using base::operator();
                };
                
                struct apply: activity<apply>{
                    using activity<apply>::activity;
                    using activity<apply>::operator();
                    
                    auto sql(){ return std::move(activity<apply>::generate()); }
                    void operator()(){ activity<apply>::query(sql()); }
                    
                    static std::string pretty(){
                        udho::pretty::printer printer;
                        printer.substitute<RelationT>();
                        printer.substitute<SuccessT>();
                        printer.substitute_all<GroupColumn...>();
                        printer.substitute<OrderedFieldT>();
                        return udho::pretty::demangle<apply>(printer);
                    }
                };
                
                template <int Limit, int Offset = 0>
                struct limit{
                    typedef typename ordered_::template limit<Limit, Offset> limit_;
                    
                    template <typename DerivedT>
                    struct activity: limit_::template activity<DerivedT>{
                        typedef typename limit_::template activity<DerivedT> limit_activity;
                        typedef typename limit_::template generators<RelationT> generators;
                        
                        generators generate;
                        
                        template <typename CollectorT, typename... Args>
                        activity(CollectorT collector, pg::connection::pool& pool, boost::asio::io_service& io, const Args&... args): limit_activity(collector, pool, io, args...), generate(limit_activity::result, limit_activity::with(), limit_activity::limited()) {}
                        template <typename ContextT, typename... T, typename... Args>
                        activity(pg::controller<ContextT, T...>& ctrl, const Args&... args): limit_activity(ctrl, args...), generate(limit_activity::result, limit_activity::with(), limit_activity::limited()){}
                        
                        using limit_activity::operator();
                    };
                    
                    struct apply: activity<apply>{
                        using activity<apply>::activity;
                        using activity<apply>::operator();
                        
                        auto sql(){ return std::move(activity<apply>::generate()); }
                        void operator()(){ activity<apply>::query(sql()); }
                        static std::string pretty(){
                            udho::pretty::printer printer;
                            printer.substitute<RelationT>();
                            printer.substitute<SuccessT>();
                            printer.substitute_all<GroupColumn...>();
                            printer.substitute<OrderedFieldT>();
                            return udho::pretty::demangle<apply>(printer);
                        }
                    };
                };
            };

            template <typename FieldT>
            using descending = ordered<pg::descending<FieldT>>;
            template <typename FieldT>
            using ascending = ordered<pg::ascending<FieldT>>;
        };
        
        template <typename... Fields>
        struct with{
            typedef typename select_::template with<Fields...> with_;
            
            template <typename DerivedT>
            struct activity: with_::template activity<DerivedT>{
                typedef typename with_::template activity<DerivedT> base;
                typedef typename with_::template generators<RelationT> generators;
                
                generators generate;
                
                template <typename CollectorT, typename... Args>
                activity(CollectorT collector, pg::connection::pool& pool, boost::asio::io_service& io, const Args&... args): base(collector, pool, io, args...), generate(base::result, base::with()) {}
                template <typename ContextT, typename... T, typename... Args>
                activity(pg::controller<ContextT, T...>& ctrl, const Args&... args): base(ctrl, args...), generate(base::result, base::with()){}
                
                using base::operator();
            };
            
            struct apply: activity<apply>{
                using activity<apply>::activity;
                using activity<apply>::operator();
                
                auto sql(){ return std::move(activity<apply>::generate()); }
                void operator()(){ activity<apply>::query(sql()); }
                static std::string pretty(){
                    udho::pretty::printer printer;
                    printer.substitute<RelationT>();
                    printer.substitute<SuccessT>();
                    printer.substitute_all<Fields...>();
                    return udho::pretty::demangle<apply>(printer);
                }
            };
            
            template <int Limit, int Offset = 0>
            struct limit{
                typedef typename with_::template limit<Limit, Offset> limit_;
                
                template <typename DerivedT>
                struct activity: limit_::template activity<DerivedT>{
                    typedef typename limit_::template activity<DerivedT> limit_activity;
                    typedef typename limit_::template generators<RelationT> generators;
                    
                    generators generate;
                    
                    template <typename CollectorT, typename... Args>
                    activity(CollectorT collector, pg::connection::pool& pool, boost::asio::io_service& io, const Args&... args): limit_activity(collector, pool, io, args...), generate(limit_activity::result, limit_activity::with(), limit_activity::limited()) {}
                    template <typename ContextT, typename... T, typename... Args>
                    activity(pg::controller<ContextT, T...>& ctrl, const Args&... args): limit_activity(ctrl, args...), generate(limit_activity::result, limit_activity::with(), limit_activity::limited()){}
                    
                    using limit_activity::operator();
                };
                
                struct apply: activity<apply>{
                    using activity<apply>::activity;
                    using activity<apply>::operator();
                    
                    auto sql(){ return std::move(activity<apply>::generate()); }
                    void operator()(){ activity<apply>::query(sql()); }
                    static std::string pretty(){
                        udho::pretty::printer printer;
                        printer.substitute<RelationT>();
                        printer.substitute<SuccessT>();
                        printer.substitute_all<Fields...>();
                        return udho::pretty::demangle<apply>(printer);
                    }
                };
            };
            
            template <typename OrderedFieldT>
            struct ordered{
                typedef typename with_::template ordered<OrderedFieldT> ordered_;
                
                template <typename DerivedT>
                struct activity: ordered_::template activity<DerivedT>{
                    typedef typename ordered_::template activity<DerivedT> base;
                    typedef typename ordered_::template generators<RelationT> generators;

                    generators generate;
                    
                    template <typename CollectorT, typename... Args>
                    activity(CollectorT collector, pg::connection::pool& pool, boost::asio::io_service& io, const Args&... args): base(collector, pool, io, args...), generate(base::result, base::with()) {}
                    template <typename ContextT, typename... T, typename... Args>
                    activity(pg::controller<ContextT, T...>& ctrl, const Args&... args): base(ctrl, args...), generate(base::result, base::with()){}
                    
                    using base::operator();
                };
                
                struct apply: activity<apply>{
                    using activity<apply>::activity;
                    using activity<apply>::operator();
                    
                    auto sql(){ return std::move(activity<apply>::generate()); }
                    void operator()(){ activity<apply>::query(sql()); }
                    static std::string pretty(){
                        udho::pretty::printer printer;
                        printer.substitute<RelationT>();
                        printer.substitute<SuccessT>();
                        printer.substitute<OrderedFieldT>();
                        printer.substitute_all<Fields...>();
                        return udho::pretty::demangle<apply>(printer);
                    }
                };
                
                template <int Limit, int Offset = 0>
                struct limit{
                    typedef typename ordered_::template limit<Limit, Offset> limit_;
                    
                    template <typename DerivedT>
                    struct activity: limit_::template activity<DerivedT>{
                        typedef typename limit_::template activity<DerivedT> limit_activity;
                        typedef typename limit_::template generators<RelationT> generators;
                        
                        generators generate;
                        
                        template <typename CollectorT, typename... Args>
                        activity(CollectorT collector, pg::connection::pool& pool, boost::asio::io_service& io, const Args&... args): limit_activity(collector, pool, io, args...), generate(limit_activity::result, limit_activity::with(), limit_activity::limited()) {}
                        template <typename ContextT, typename... T, typename... Args>
                        activity(pg::controller<ContextT, T...>& ctrl, const Args&... args): limit_activity(ctrl, args...), generate(limit_activity::result, limit_activity::with(), limit_activity::limited()){}
                        
                        using limit_activity::operator();
                    };
                    
                    struct apply: activity<apply>{
                        using activity<apply>::activity;
                        using activity<apply>::operator();
                        
                        auto sql(){ return std::move(activity<apply>::generate()); }
                        void operator()(){ activity<apply>::query(sql()); }
                        static std::string pretty(){
                            udho::pretty::printer printer;
                            printer.substitute<RelationT>();
                            printer.substitute<SuccessT>();
                            printer.substitute<OrderedFieldT>();
                            printer.substitute_all<Fields...>();
                            return udho::pretty::demangle<apply>(printer);
                        }
                    };
                };
            };

            template <typename FieldT>
            using descending = ordered<pg::descending<FieldT>>;
            template <typename FieldT>
            using ascending = ordered<pg::ascending<FieldT>>;
            
            template <typename... GroupColumn>
            struct group{
                typedef typename with_::template group<GroupColumn...> group_;
                
                template <typename DerivedT>
                struct activity: group_::template activity<DerivedT>{
                    typedef typename group_::template activity<DerivedT> group_activity;
                    typedef typename group_::template generators<RelationT> generators;
                    
                    generators generate;
                    
                    template <typename CollectorT, typename... Args>
                    activity(CollectorT collector, pg::connection::pool& pool, boost::asio::io_service& io, const Args&... args): group_activity(collector, pool, io, args...), generate(group_activity::result, group_activity::with()) {}
                    template <typename ContextT, typename... T, typename... Args>
                    activity(pg::controller<ContextT, T...>& ctrl, const Args&... args): group_activity(ctrl, args...), generate(group_activity::result, group_activity::with()){}
                    
                    using group_activity::operator();
                };
                
                struct apply: activity<apply>{
                    using activity<apply>::activity;
                    using activity<apply>::operator();
                    
                    auto sql(){ return std::move(activity<apply>::generate()); }
                    void operator()(){ activity<apply>::query(sql()); }
                    static std::string pretty(){
                        udho::pretty::printer printer;
                        printer.substitute<RelationT>();
                        printer.substitute<SuccessT>();
                        printer.substitute_all<GroupColumn...>();
                        return udho::pretty::demangle<apply>(printer);
                    }
                };
                
                template <typename OrderedFieldT>
                struct ordered{
                    typedef typename group_::template ordered<OrderedFieldT> ordered_;
                    
                    template <typename DerivedT>
                    struct activity: ordered_::template activity<DerivedT>{
                        typedef typename ordered_::template activity<DerivedT> base;
                        typedef typename ordered_::template generators<RelationT> generators;

                        generators generate;
                        
                        template <typename CollectorT, typename... Args>
                        activity(CollectorT collector, pg::connection::pool& pool, boost::asio::io_service& io, const Args&... args): base(collector, pool, io, args...), generate(base::result, base::with()) {}
                        template <typename ContextT, typename... T, typename... Args>
                        activity(pg::controller<ContextT, T...>& ctrl, const Args&... args): base(ctrl, args...), generate(base::result, base::with()){}
                        
                        using base::operator();
                    };
                    
                    struct apply: activity<apply>{
                        using activity<apply>::activity;
                        using activity<apply>::operator();
                        
                        auto sql(){ return std::move(activity<apply>::generate()); }
                        void operator()(){ activity<apply>::query(sql()); }
                        static std::string pretty(){
                            udho::pretty::printer printer;
                            printer.substitute<RelationT>();
                            printer.substitute<SuccessT>();
                            printer.substitute<OrderedFieldT>();
                            printer.substitute_all<GroupColumn...>();
                            return udho::pretty::demangle<apply>(printer);
                        }
                    };
                    
                    template <int Limit, int Offset = 0>
                    struct limit{
                        typedef typename ordered_::template limit<Limit, Offset> limit_;
                        
                        template <typename DerivedT>
                        struct activity: limit_::template activity<DerivedT>{
                            typedef typename limit_::template activity<DerivedT> limit_activity;
                            typedef typename limit_::template generators<RelationT> generators;
                            
                            generators generate;
                            
                            template <typename CollectorT, typename... Args>
                            activity(CollectorT collector, pg::connection::pool& pool, boost::asio::io_service& io, const Args&... args): limit_activity(collector, pool, io, args...), generate(limit_activity::result, limit_activity::with(), limit_activity::limited()) {}
                            template <typename ContextT, typename... T, typename... Args>
                            activity(pg::controller<ContextT, T...>& ctrl, const Args&... args): limit_activity(ctrl, args...), generate(limit_activity::result, limit_activity::with(), limit_activity::limited()){}
                            
                            using limit_activity::operator();
                        };
                        
                        struct apply: activity<apply>{
                            using activity<apply>::activity;
                            using activity<apply>::operator();
                            
                            auto sql(){ return std::move(activity<apply>::generate()); }
                            void operator()(){ activity<apply>::query(sql()); }
                            static std::string pretty(){
                                udho::pretty::printer printer;
                                printer.substitute<RelationT>();
                                printer.substitute<SuccessT>();
                                printer.substitute<SuccessT>();
                                printer.substitute_all<OrderedFieldT>();
                                printer.substitute_all<GroupColumn...>();
                                return udho::pretty::demangle<apply>(printer);
                            }
                        };
                    };
                };

                template <typename FieldT>
                using descending = ordered<pg::descending<FieldT>>;
                template <typename FieldT>
                using ascending = ordered<pg::ascending<FieldT>>;
            };
        };
    };
    
    template <typename SchemaT>
    struct update{
        typedef typename pg::update<SchemaT> update_;
        
        template <typename DerivedT>
        struct activity: update_::template activity<DerivedT>{
            typedef typename update_::template activity<DerivedT> base;
            typedef typename update_::template generators<RelationT> generators;
            
            generators generate;
            
            template <typename CollectorT, typename... Args>
            activity(CollectorT collector, pg::connection::pool& pool, boost::asio::io_service& io, Args&&... args): base(collector, pool, io, std::forward<Args>(args)...), generate(base::schema()) {}
            template <typename ContextT, typename... T, typename... Args>
            activity(pg::controller<ContextT, T...>& ctrl, Args&&... args): base(ctrl, std::forward<Args>(args)...), generate(base::schema()) {}
            
            using base::operator();
        };
        
        struct apply: activity<apply>{
            using activity<apply>::activity;
            using activity<apply>::operator();
            
            static std::string pretty(){
                udho::pretty::printer printer;
                printer.substitute<RelationT>();
                printer.substitute<SchemaT>();
                return udho::pretty::demangle<apply>(printer);
            }
            
            auto sql(){ return std::move(activity<apply>::generate()); }
            void operator()(){ activity<apply>::query(sql()); }
        };
        
        template <typename... WithFields>
        struct with{
            
            typedef typename update_::template with<WithFields...> with_;
            
            template <typename DerivedT>
            struct activity: with_::template activity<DerivedT>{
                typedef typename with_::template activity<DerivedT> base;
                typedef typename with_::template generators<RelationT> generators;
                
                generators generate;
                
                template <typename CollectorT, typename... Args>
                activity(CollectorT collector, pg::connection::pool& pool, boost::asio::io_service& io, Args&&... args): base(collector, pool, io, std::forward<Args>(args)...), generate(base::schema(), base::with()) {}
                template <typename ContextT, typename... T, typename... Args>
                activity(pg::controller<ContextT, T...>& ctrl, Args&&... args): base(ctrl, std::forward<Args>(args)...), generate(base::schema(), base::with()) {}
                
                using base::operator();
            };
            
            struct apply: activity<apply>{
                using activity<apply>::activity;
                using activity<apply>::operator();
                
                static std::string pretty(){
                    udho::pretty::printer printer;
                    printer.substitute<RelationT>();
                    printer.substitute<SchemaT>();
                    printer.substitute_all<WithFields...>();
                    return udho::pretty::demangle<apply>(printer);
                }
                
                auto sql(){ return std::move(activity<apply>::generate()); }
                void operator()(){ activity<apply>::query(sql()); }
            };
            
            template <typename... ReturningFields>
            struct returning{
                
                typedef typename with_::template returning<ReturningFields...> returning_;
                
                template <typename DerivedT>
                struct activity: returning_::template activity<DerivedT>{
                    typedef typename returning_::template activity<DerivedT> base;
                    typedef typename returning_::template generators<RelationT> generators;
                    
                    generators generate;
                    
                    template <typename CollectorT, typename... Args>
                    activity(CollectorT collector, pg::connection::pool& pool, boost::asio::io_service& io, Args&&... args): base(collector, pool, io, std::forward<Args>(args)...), generate(base::schema(), base::with()) {}
                    template <typename ContextT, typename... T, typename... Args>
                    activity(pg::controller<ContextT, T...>& ctrl, Args&&... args): base(ctrl, std::forward<Args>(args)...), generate(base::schema(), base::with()) {}
                    
                    using base::operator();
                };
                
                struct apply: activity<apply>{
                    using activity<apply>::activity;
                    using activity<apply>::operator();
                    
                    static std::string pretty(){
                        udho::pretty::printer printer;
                        printer.substitute<RelationT>();
                        printer.substitute<SchemaT>();
                        printer.substitute_all<WithFields...>();
                        printer.substitute_all<ReturningFields...>();
                        return udho::pretty::demangle<apply>(printer);
                    }
                    
                    auto sql(){ return std::move(activity<apply>::generate()); }
                    void operator()(){ activity<apply>::query(sql()); }
                };
                
            };
            
        };
        
        template <typename... ReturningFields>
        struct returning{
            
            typedef typename update_::template returning<ReturningFields...> returning_;
            
            template <typename DerivedT>
            struct activity: update_::template activity<DerivedT>{
                typedef typename update_::template activity<DerivedT> base;
                typedef typename update_::template generators<RelationT> generators;
                
                generators generate;
                
                template <typename CollectorT, typename... Args>
                activity(CollectorT collector, pg::connection::pool& pool, boost::asio::io_service& io, Args&&... args): base(collector, pool, io, std::forward<Args>(args)...), generate(base::schema()) {}
                template <typename ContextT, typename... T, typename... Args>
                activity(pg::controller<ContextT, T...>& ctrl, Args&&... args): base(ctrl, std::forward<Args>(args)...), generate(base::schema()) {}
                
                using base::operator();
            };
            
            struct apply: activity<apply>{
                using activity<apply>::activity;
                using activity<apply>::operator();
                
                static std::string pretty(){
                    udho::pretty::printer printer;
                    printer.substitute<RelationT>();
                    printer.substitute<SchemaT>();
                    printer.substitute_all<ReturningFields...>();
                    return udho::pretty::demangle<apply>(printer);
                }
                
                auto sql(){ return std::move(activity<apply>::generate()); }
                void operator()(){ activity<apply>::query(sql()); }
            };
            
        };
    };
    
    template <typename SchemaT>
    struct insert{
        typedef typename pg::insert<SchemaT> insert_;
        
        template <typename DerivedT>
        struct activity: insert_::template activity<DerivedT>{
            typedef typename insert_::template activity<DerivedT> base;
            typedef typename insert_::template generators<RelationT> generators;
            
            generators generate;
            
            template <typename CollectorT, typename... Args>
            activity(CollectorT collector, pg::connection::pool& pool, boost::asio::io_service& io, Args&&... args): base(collector, pool, io, std::forward<Args>(args)...), generate(base::schema()) {}
            template <typename ContextT, typename... T, typename... Args>
            activity(pg::controller<ContextT, T...>& ctrl, Args&&... args): base(ctrl, std::forward<Args>(args)...), generate(base::schema()) {}
            
            using base::operator();
        };
        
        struct apply: activity<apply>{
            using activity<apply>::activity;
            using activity<apply>::operator();
            
            static std::string pretty(){
                udho::pretty::printer printer;
                printer.substitute<RelationT>();
                printer.substitute<SchemaT>();
                return udho::pretty::demangle<apply>(printer);
            }
            
            auto sql(){ return std::move(activity<apply>::generate()); }
            void operator()(){ activity<apply>::query(sql()); }
        };
        
        template <typename... ReturningFields>
        struct returning{
            
            typedef typename insert_::template returning<ReturningFields...> returning_;
            
            template <typename DerivedT>
            struct activity: returning_::template activity<DerivedT>{
                typedef typename returning_::template activity<DerivedT> base;
                typedef typename returning_::template generators<RelationT> generators;
                
                generators generate;
                
                template <typename CollectorT, typename... Args>
                activity(CollectorT collector, pg::connection::pool& pool, boost::asio::io_service& io, Args&&... args): base(collector, pool, io, std::forward<Args>(args)...), generate(base::schema()) {}
                template <typename ContextT, typename... T, typename... Args>
                activity(pg::controller<ContextT, T...>& ctrl, Args&&... args): base(ctrl, std::forward<Args>(args)...), generate(base::schema()) {}
                
                using base::operator();
            };
            
            struct apply: activity<apply>{
                using activity<apply>::activity;
                using activity<apply>::operator();
                
                static std::string pretty(){
                    udho::pretty::printer printer;
                    printer.substitute<SchemaT>();
                    printer.substitute<RelationT>();
                    printer.substitute_all<ReturningFields...>();
                    return udho::pretty::demangle<apply>(printer);
                }
                
                auto sql(){ return std::move(activity<apply>::generate()); }
                void operator()(){ activity<apply>::query(sql()); }
            };
            
        };
    };
    
    template <typename... Fields>
    struct remove{
        typedef typename pg::remove<Fields...> remove_;
        
        template <typename DerivedT>
        struct activity: remove_::template activity<DerivedT>{
            typedef typename remove_::template activity<DerivedT> base;
            typedef typename remove_::template generators<RelationT> generators;
            
            generators generate;
            
            template <typename CollectorT, typename... Args>
            activity(CollectorT collector, pg::connection::pool& pool, boost::asio::io_service& io, Args&&... args): base(collector, pool, io, std::forward<Args>(args)...), generate(base::with()) {}
            template <typename ContextT, typename... T, typename... Args>
            activity(pg::controller<ContextT, T...>& ctrl, Args&&... args): base(ctrl, std::forward<Args>(args)...), generate(base::with()){}
            
            using base::operator();
        };
        
        struct apply: activity<apply>{
            using activity<apply>::activity;
            using activity<apply>::operator();
            
            static std::string pretty(){
                udho::pretty::printer printer;
                printer.substitute<RelationT>();
                printer.substitute_all<Fields...>();
                return udho::pretty::demangle<apply>(printer);
            }
            
            auto sql(){ return std::move(activity<apply>::generate()); }
            void operator()(){ activity<apply>::query(sql()); }
        };
    };
};
    
}
}
}

#endif // UDHO_DB_PG_CRUD_BUILDER_H
