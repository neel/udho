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

#ifndef UDHO_DB_PG_TESTS_COMMON_H
#define UDHO_DB_PG_TESTS_COMMON_H

namespace udho{
namespace db{
namespace pg{
namespace tests{

struct sql{
    std::string _query;

    sql() = default;
    explicit sql(const char* q): _query(q) {}
    sql(const sql&) = default;

    std::string query() const {
        std::string q = boost::algorithm::trim_copy(_query);
        return std::regex_replace(q, std::regex("\\s+"), " ");
    }

    friend bool operator==(const sql& lhs, const sql& rhs);
    friend std::ostream& operator<<(std::ostream& stream, const sql& s);
    friend sql operator%(const sql&, const char* str);
    friend sql operator%(const char* str, const sql&);
};

bool operator==(const sql& lhs, const sql& rhs){
    return lhs.query() == rhs.query();
}

sql operator%(const sql&, const char* str){
    return sql(str);
}

sql operator%(const char* str, const sql&){
    return sql(str);
}

std::ostream& operator<<(std::ostream& stream, const sql& s){
    stream << s.query();
    return stream;
}

}
}
}
}

#define SQL_EXPECT_SAME(x, y) CHECK(udho::db::pg::tests::sql() % x.text().c_str() == y % udho::db::pg::tests::sql())
#define SQL_EXPECT(x, y, T) CHECK(udho::db::pg::tests::sql() % x.text().c_str() == y % udho::db::pg::tests::sql()); CHECK((x.params() == T))


#endif // UDHO_DB_PG_TESTS_COMMON_H
