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

#ifndef UDHO_DB_PG_CRUD_CREATE_H
#define UDHO_DB_PG_CRUD_CREATE_H

#include <udho/db/pg/activities/activity.h>
#include <udho/db/common/none.h>
#include <udho/db/pg/crud/fwd.h>
#include <udho/db/pg/generators/create.h>
#include <udho/db/pg/generators/drop.h>

namespace udho{
namespace db{
namespace pg{

template <typename RelationT>
struct ddl{
    using create_generator = pg::generators::create_<RelationT>;
    using drop_generator   = pg::generators::drop<RelationT>;

    /**
     * \tparam CreateGenerator either of pg::generators::create_<RelationT>::all, pg::generators::create_<RelationT>::template except<Fields...>, pg::generators::create_<RelationT>::template only<Fields...>
     */
    template <typename CreateGenerator>
    struct if_exists_{
        using create_type  = CreateGenerator;

        struct drop{
            struct generators{
                auto operator()() {
                    using namespace ozo::literals;
                    drop_generator d;
                    create_type    c;
                    return d() + "; "_SQL + c();
                }
            };
        };

        struct skip{
            struct generators{
                auto operator()() {
                    create_type c;
                    return c();
                }
            };
        };
    };

    /**
     * \tparam IfExistsT instantiation of template if_exists_
     */
    template <typename IfExistsT>
    struct basic_create{
        struct if_exists{
            struct drop{
                template <typename DerivedT>
                struct activity: pg::activity<DerivedT, db::none>{
                    typedef pg::activity<DerivedT, db::none> activity_type;

                    typename IfExistsT::drop::generators generate;

                    template <typename CollectorT, typename... Args>
                    activity(CollectorT collector, pg::connection::pool& pool, boost::asio::io_service& io):
                        activity_type(collector, pool, io)
                        {}
                    template <typename ContextT, typename... T, typename... Args>
                    activity(pg::controller<ContextT, T...>& ctrl): activity(ctrl.data(), ctrl.pool(), ctrl.io()){}

                    using activity_type::operator();
                };
                struct apply: activity<apply>{
                    using activity<apply>::activity;
                    using activity<apply>::operator();

                    auto sql(){ return std::move(activity<apply>::generate()); }
                    void operator()(){ activity<apply>::query(sql()); }
                    static std::string pretty(){
                        udho::pretty::printer printer;
                        printer.substitute<RelationT>();
                        return udho::pretty::demangle<apply>(printer);
                    }
                };
            };
            struct skip{
                template <typename DerivedT>
                struct activity: pg::activity<DerivedT, db::none>{
                    typedef pg::activity<DerivedT, db::none> activity_type;

                    typename IfExistsT::skip::generators generate;

                    template <typename CollectorT, typename... Args>
                    activity(CollectorT collector, pg::connection::pool& pool, boost::asio::io_service& io):
                        activity_type(collector, pool, io)
                        {}
                    template <typename ContextT, typename... T, typename... Args>
                    activity(pg::controller<ContextT, T...>& ctrl): activity(ctrl.data(), ctrl.pool(), ctrl.io()){}

                    using activity_type::operator();
                };
                struct apply: activity<apply>{
                    using activity<apply>::activity;
                    using activity<apply>::operator();

                    auto sql(){ return std::move(activity<apply>::generate()); }
                    void operator()(){ activity<apply>::query(sql()); }
                    static std::string pretty(){
                        udho::pretty::printer printer;
                        printer.substitute<RelationT>();
                        return udho::pretty::demangle<apply>(printer);
                    }
                };
            };
        };
        template <typename... Fields>
        using except = basic_create<if_exists_<typename pg::generators::create_<RelationT>::template except<Fields...>>>;
        template <typename... Fields>
        using only = basic_create<if_exists_<typename pg::generators::create_<RelationT>::template only<Fields...>>>;
    };
    /**
     * create::if_exists::skip::apply
     * create::if_exists::drop::apply
     */
    using create = basic_create<if_exists_<typename pg::generators::create_<RelationT>::all>>;

    /**
     * drop::apply
     */
    struct drop{
        template <typename DerivedT>
        struct activity: pg::activity<DerivedT, db::none>{
            typedef pg::activity<DerivedT, db::none> activity_type;

            drop_generator generate;

            template <typename CollectorT, typename... Args>
            activity(CollectorT collector, pg::connection::pool& pool, boost::asio::io_service& io):
                activity_type(collector, pool, io)
                {}
            template <typename ContextT, typename... T, typename... Args>
            activity(pg::controller<ContextT, T...>& ctrl): activity(ctrl.data(), ctrl.pool(), ctrl.io()){}

            using activity_type::operator();
        };
        struct apply: activity<apply>{
            using activity<apply>::activity;
            using activity<apply>::operator();

            auto sql(){ return std::move(activity<apply>::generate()); }
            void operator()(){ activity<apply>::query(sql()); }
            static std::string pretty(){
                udho::pretty::printer printer;
                printer.substitute<RelationT>();
                return udho::pretty::demangle<apply>(printer);
            }
        };
    };
};

}
}
}


#endif // UDHO_DB_PG_CRUD_CREATE_H
