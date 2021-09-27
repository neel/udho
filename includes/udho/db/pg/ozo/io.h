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

#ifndef UDHO_DB_PG_IO_H
#define UDHO_DB_PG_IO_H

#include <string>
#include <ozo/connection_pool.h>
#include <ozo/connection_info.h>
#include <udho/db/pg/schema/column.h>
#include <udho/db/pg/schema/basic.h>
#include <system_error>
#include <boost/format.hpp>

#ifdef WITH_JSON_NLOHMANN
#include <nlohmann/json.hpp>
#endif 

namespace ozo {

#ifdef WITH_JSON_NLOHMANN

template <>
struct size_of_impl<nlohmann::json> {
    static auto apply(const nlohmann::json& v) noexcept {
        return v.size();
    }
};

template <>
struct send_impl<nlohmann::json> {
    template <typename OidMap>
    static ostream& apply(ostream& out, const OidMap&, const nlohmann::json& in) {
        return write(out, in.dump());
    }
};

template <>
struct recv_impl<nlohmann::json> {
    template <typename OidMap>
    static istream& apply(istream& in, size_type size, const OidMap&, nlohmann::json& out) {
        std::string raw;
        raw.resize(size);
        istream& res = read(in, raw);
        out = nlohmann::json::parse(raw);
        return res;
    }
};

#endif 

} 

namespace udho{namespace db{namespace pg{namespace io{
    
template <typename RecordT>
struct receive{
    static void apply(pg::connection::pool::connection_type conn, const ozo::result::row& row, RecordT& record){
        ozo::recv_row(row, conn->oid_map(), record);
    }
};

template <typename... Fields>
struct receive<udho::db::pg::basic_schema<Fields...>>{
    static void apply(pg::connection::pool::connection_type conn, const ozo::result::row& row, udho::db::pg::basic_schema<Fields...>& record){
        std::size_t index = 0;
        record.visit([&](auto& elem) mutable {
            std::string column = elem.ozo_name().text().c_str();
            try{
                ozo::result::value value = row.at(index);
                if(!value.is_null()){
                    ozo::recv(value, conn->oid_map(), elem.value());
                }else{
                    elem.null(true);
                }
            }catch(const ozo::system_error& error){
                throw ozo::system_error(error.code(), (boost::format("Error receiving column %1% (%2%) of recordset with message: %3%") % index % column % error.what()).str());
            }
            index++;
        });
    }
};
    
}}}}


#ifdef WITH_JSON_NLOHMANN
OZO_PG_BIND_TYPE(nlohmann::json, "json")
#endif

// OZO_STRONG_TYPEDEF(std::string, varchar)
// OZO_PG_BIND_TYPE(varchar, "varchar")

#endif // UDHO_DB_PG_IO_H
