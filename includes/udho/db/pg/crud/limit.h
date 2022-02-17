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

#ifndef UDHO_DB_PG_CRUD_LIMIT_H
#define UDHO_DB_PG_CRUD_LIMIT_H

#include <cstdint>
#include <udho/db/pg/crud/fwd.h>

namespace udho{
namespace db{
namespace pg{
    
/**
 * @brief An utility to specify the limit and offset, used in combination with the limit generator
 * 
 * @tparam Limit 
 * @tparam Offset 
 * 
 * @ingroup generators
 */
template <int Limit, int Offset>
struct limited{
    limited(): _limit(Limit), _offset(Offset){}
    
    /**
     * @brief set the limit
     * 
     * @param limit 
     */
    void limit(const std::int64_t& limit) { _limit = limit; }
    /**
     * @brief get the limit already set
     * 
     * @return const std::int64_t& 
     */
    const std::int64_t& limit() const { return _limit; }
    
    /**
     * @brief set the offset
     * 
     * @param offset 
     */
    void offset(const std::int64_t& offset) { _offset = offset; }
    /**
     * @brief get the offset altready set
     * 
     * @return const std::int64_t& 
     */
    const std::int64_t& offset() const { return _offset; }
    
    private:
        std::int64_t _limit;
        std::int64_t _offset;
};
    
}
}
}

#endif // UDHO_DB_PG_CRUD_LIMIT_H
