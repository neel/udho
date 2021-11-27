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

#ifndef UDHO_DB_PG_ACTIVITY_BASIC_H
#define UDHO_DB_PG_ACTIVITY_BASIC_H

#include <ozo/connection_info.h>
#include <ozo/connection_pool.h>
#include <ozo/request.h>
#include <ozo/result.h>
#include <udho/db/pg/ozo/connection.h>
#include <udho/db/pg/ozo/io.h>
#include <udho/db/common/common.h>
#include <udho/activities/activities.h>
#include <udho/db/pg/activities/failure.h>
#include <udho/db/pg/activities/controller.h>
#include <boost/bind/bind.hpp>
#include <boost/beast/http/message.hpp>
#include <udho/db/pg/ozo/fwd.h>

namespace udho{
namespace db{
namespace pg{

/**
 * @brief Basic activity for asynchronous postgresql queries
 * The derived class's operator() is supposed to construct an OZO query and pass that to the basic_activity::query() method.
 * The provided query is invoked asynchronously. If that query invocation fails or if the query results into some postgresql 
 * error then the activity fails and yields failure data (pg::failure) which consists of information related to that failure.
 * If the query runs successfully then first the result(s) are stored in an std::vector<RowT>. Which is then transformed into
 * 
 * @see pg::activity
 * @tparam DerivedT The Derived activity class 
 * @tparam SuccessT Type of success result, defaults to db::none, If the provided SuccessT has a typedef `data_type` then that SuccessT is used to store the successful result(s) of the activity. Otherwise the type db::result<SuccessT> is used
 * @tparam RowT By default RowT is  same as SuccessT. However usercode may provide a custom RowT
 */
template <typename DerivedT, typename SuccessT = db::none, typename RowT = typename std::conditional<db::detail::HasDataType<SuccessT>::value, SuccessT, db::result<SuccessT>>::type::data_type>
struct basic_activity: udho::activity<DerivedT, typename std::conditional<db::detail::HasDataType<SuccessT>::value, SuccessT, db::result<SuccessT>>::type, pg::failure>{
    typedef udho::activity<DerivedT, typename std::conditional<db::detail::HasDataType<SuccessT>::value, SuccessT, db::result<SuccessT>>::type, pg::failure> base;
    typedef basic_activity<DerivedT, SuccessT, RowT> self_type;
    typedef typename std::conditional<db::detail::HasDataType<SuccessT>::value, SuccessT, db::result<SuccessT>>::type success_type;
    typedef RowT row_type;
    typedef typename success_type::data_type record_type;
    typedef DerivedT derived_type;
    typedef db::detail::transform<derived_type, record_type, row_type> transformer_type;
    typedef std::vector<row_type> container_type;
    typedef boost::transform_iterator<transformer_type, typename container_type::const_iterator> iterator_type;
        
    enum {generated = false};

    template <typename CollectorT>
    basic_activity(CollectorT c, pg::connection::pool& pool, boost::asio::io_service& io): base(c), _pool(pool), _io(io), _transformer(static_cast<derived_type&>(*this)){}
    
    template <typename ContextT, typename... T>
    basic_activity(pg::controller<ContextT, T...>& ctrl): basic_activity(ctrl.data(), ctrl.pool(), ctrl.io()){}
    
    template <typename QueryT>
    void query(QueryT&& query){
        using namespace std::placeholders;
        try{
            ozo::request(_pool[_io], query, std::ref(_results), std::bind(&self_type::resolve, base::self(), _1, _2));
        }catch(const std::exception& ex){
            derived_type& self = static_cast<derived_type&>(*this);
            failure data = failure::make(self);
            data.error   = boost::system::errc::make_error_code(boost::system::errc::state_not_recoverable);
            data.reason  = std::string(ex.what());

            base::failure(data);
        }
    }
    
    void resolve(ozo::error_code ec, pg::connection::pool::connection_type conn){
        derived_type& self = static_cast<derived_type&>(*this);
        if(ec){
            std::stringstream stream;
            stream << ozo::error_message(conn);
            if (!ozo::is_null_recursive(conn)) {
                stream << " | " << ozo::get_error_context(conn);
            }
            
            failure data = failure::make(self);
            data.error   = ec;
            data.reason  = stream.str();

            base::failure(data);
        }else{
            try{
                for(const auto& row: _results){
                    row_type record;
                    udho::db::pg::io::receive<row_type>::apply(conn, row, record);
                    _rows.push_back(record);
                }
            }catch(const ozo::system_error& error){
                failure data = failure::make(self);
                data.error   = error.code();
                data.reason  = error.what();

                base::failure(data);
                return;
            }
            
            iterator_type begin(_rows.cbegin(), _transformer), end(_rows.cend(), _transformer);
            db::detail::processor<derived_type, success_type> processor(static_cast<derived_type&>(*this));
            processor.process(begin, end);
        }
    }
    
    void clear(){
        _rows.clear();
    }
    
    private:
        pg::connection::pool&    _pool;
        boost::asio::io_service& _io;
        transformer_type         _transformer;
        container_type           _rows;
        ozo::result              _results;
};

/**
 * @brief Basic activity for asynchronous postgresql queries specialized for queries that does not respond with any result set
 * The derived class's operator() is supposed to construct an OZO query and pass that to the basic_activity::query() method.
 * The provided query is invoked asynchronously. If that query invocation fails or if the query results into some postgresql 
 * error then the activity fails and yields failure data (pg::failure) which consists of information related to that failure.
 * @see pg::activity
 * @tparam DerivedT 
 */
template <typename DerivedT>
struct basic_activity<DerivedT, db::none>: udho::activity<DerivedT, db::none, pg::failure>{
    typedef udho::activity<DerivedT, db::none, pg::failure> base;
    typedef basic_activity<DerivedT> self_type;
    typedef db::none success_type;
    typedef DerivedT derived_type;
        
    template <typename CollectorT>
    basic_activity(CollectorT collector, pg::connection::pool& pool, boost::asio::io_service& io): base(collector), _pool(pool), _io(io){}
    template <typename ContextT, typename... T>
    basic_activity(pg::controller<ContextT, T...>& ctrl): basic_activity(ctrl.data(), ctrl.pool(), ctrl.io()){}
    
    enum {generated = false};

    template <typename QueryT>
    void query(QueryT&& query){
        derived_type& self = static_cast<derived_type&>(*this);
        try{
            ozo::request(_pool[_io], query, std::ref(_result), boost::bind(&self_type::resolve, base::self(), boost::placeholders::_1, boost::placeholders::_2));
        }catch(const std::exception& ex){
            failure data = failure::make(self);
            data.error  = boost::system::errc::make_error_code(boost::system::errc::state_not_recoverable);
            data.reason = std::string(ex.what());

            base::failure(data);
        }
    }
    
    void resolve(ozo::error_code ec, pg::connection::pool::connection_type conn){
        derived_type& self = static_cast<derived_type&>(*this);
        if(ec){
            std::stringstream stream;
            stream << ozo::error_message(conn);
            if (!ozo::is_null_recursive(conn)) {
                stream << " | " << ozo::get_error_context(conn);
            }
            
            failure data = failure::make(self);
            data.error  = ec;
            data.reason = stream.str();

            base::failure(data);
        }else{
            db::none data;
            self(data);
        }
    }
    void clear(){}
    
    private:
        pg::connection::pool&    _pool;
        boost::asio::io_service& _io;
        ozo::result              _result;
};
    
}
}
}

#endif // UDHO_DB_PG_ACTIVITY_BASIC_H

