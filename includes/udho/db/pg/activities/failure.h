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

#ifndef UDHO_ACTIVITIES_DB_PG_ACTIVITY_FAILURE_H
#define UDHO_ACTIVITIES_DB_PG_ACTIVITY_FAILURE_H

#include <sstream>
#include <string>
#include <ozo/error.h>
#include <udho/db/pretty/type.h>
#include <udho/page.h>
#include <boost/property_tree/detail/xml_parser_utils.hpp>

namespace udho{
namespace db{
namespace pg{

namespace detail{

GENERATE_HAS_MEMBER(sql);

template <typename ActivityT, bool HasSQL = has_member_sql<ActivityT>::value>
struct extract_sql_text_if_available{
    static std::string sql(ActivityT&){
        return "SQL UNAVAILABLE";
    }
};

template <typename ActivityT>
struct extract_sql_text_if_available<ActivityT, true>{
    static std::string sql(ActivityT& activity){
        return activity.sql().text().c_str();
    }
};

};

struct failure{
    ozo::error_code error;
    std::string     reason;
    std::string     origin;
    std::string     sql;

    template <typename ActivityT>
    static failure make(ActivityT& activity){
        failure f;
        f.origin = db::pretty::indent<ActivityT>();
        f.sql    = detail::extract_sql_text_if_available<ActivityT>::sql(activity);
        return f;
    }
};

struct exception: udho::exceptions::http_error{
    failure _failure;

    inline explicit exception(const failure& f, const std::string& message): udho::exceptions::http_error(boost::beast::http::status::internal_server_error, message), _failure(f){}

    inline void decorate(udho::exceptions::visual::page& p) const{
        std::stringstream stream;
        stream << "<div class='db-error'>"
               <<     "<div class='db-prop error'>  <div class='db-key'>Error:  </div> <div class='db-value'><code>"  << _failure.error  << "</code></div></div>"
               <<     "<div class='db-prop reason'> <div class='db-key'>Reason: </div> <div class='db-value'><code>"  << boost::property_tree::xml_parser::encode_char_entities(_failure.reason) << "</code></div></div>"
               <<     "<div class='db-prop origin'> <div class='db-key'>Source: </div> <div class='db-value'><code>"  << boost::property_tree::xml_parser::encode_char_entities(_failure.origin) << "</code></div></div>"
               <<     "<div class='db-prop sql'>    <div class='db-key'>SQL:    </div> <div class='db-value'><code>"  << boost::property_tree::xml_parser::encode_char_entities(_failure.sql)    << "</code></div></div>"
               << "</div>";
        udho::exceptions::visual::block db_block("Database", stream.str());
        p.add_block(db_block);
    }
};

}
}
}

#endif // UDHO_ACTIVITIES_DB_PG_ACTIVITY_FAILURE_H