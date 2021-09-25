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

#ifndef UDHO_ACTIVITIES_DB_PG_FEATURES_H
#define UDHO_ACTIVITIES_DB_PG_FEATURES_H

#include <cstdint>

namespace udho{
namespace db{
namespace pg{
    
template <typename FeatureT>
struct feature_;

struct limit{};
struct offset{};
struct last{};

template <>
struct feature_<limit>{
    feature_(const std::int64_t& limit = 0): _limit(limit){}
    void limit(const std::int64_t& limit) { _limit = limit; }
    std::int64_t limit() const { return _limit; }
    
    private:
        std::int64_t _limit;
};

template <>
struct feature_<offset>{
    feature_(const std::int64_t& offset = 0): _offset(offset){}
    void offset(const std::int64_t& offset) { _offset = offset; }
    std::int64_t offset() const { return _offset; }
    
    private:
        std::int64_t _offset;
};

template <>
struct feature_<last>{
    feature_(const std::int64_t& last = 0): _last(last) {}
    void last(const std::int64_t& last) { _last = last; }
    std::int64_t last() const { return _last; }
    
    private:
        std::int64_t _last;
};

template <typename... Features>
struct features: feature_<Features>...{
    template <typename... Args>
    features(const Args&... args): feature_<Features>(args)...{}
};


/// new API

namespace extra{
    
template <typename FieldT, bool IsAscending = true>
struct ascending{};

template <typename FieldT>
struct ascending<FieldT, false>{};

template <typename FieldT>
using descending = ascending<FieldT, false>;
    
template <int Limit = -1, int Offset=0>
struct limited{
    limited(): _limit(Limit), _offset(Offset){}
    
    void limit(const std::int64_t& limit) { _limit = limit; }
    const std::int64_t& limit() const { return _limit; }
    
    void offset(const std::int64_t& offset) { _offset = offset; }
    const std::int64_t& offset() const { return _offset; }
    
    private:
        std::int64_t _limit;
        std::int64_t _offset;
};

template <typename FieldT>
struct ordered_by;

template <typename FieldT, bool IsAscending>
struct ordered_by<ascending<FieldT, IsAscending>>{};

template <typename... FeaturesT>
struct featuring: FeaturesT...{};

}
    
}
}
}

#endif // UDHO_ACTIVITIES_DB_PG_FEATURES_H
