// SPDX-FileCopyrightText: 2026 Neel Basu <email>
// SPDX-License-Identifier: BSD-3-Clause

#ifndef UDHO_VIEW_BRIDGES_SOURCE_MAP_H
#define UDHO_VIEW_BRIDGES_SOURCE_MAP_H

#include <cstddef>
#include <optional>
#include <stdexcept>
#include <utility>

#include <boost/icl/interval.hpp>
#include <boost/icl/interval_map.hpp>

namespace udho{
namespace view{
namespace data{
namespace bridges{

/**
 * @brief Inclusive line range in the original view source.
 */
struct source_span{
    std::size_t begin = 0;
    std::size_t end   = 0;

    inline bool valid() const noexcept { return begin > 0 && end >= begin; }

    friend inline bool operator==(const source_span& lhs, const source_span& rhs) noexcept {
        return lhs.begin == rhs.begin && lhs.end == rhs.end;
    }

    friend inline bool operator!=(const source_span& lhs, const source_span& rhs) noexcept {
        return !(lhs == rhs);
    }

    // Required by boost::icl's default codomain combiner. source_map::add()
    // rejects overlapping generated intervals, so unlike values must never be
    // combined during normal operation.
    inline source_span& operator+=(const source_span& other) {
        if(*this != other){
            throw std::logic_error{"overlapping Lua source mappings"};
        }
        return *this;
    }
};

/**
 * @brief Maps half-open generated Lua line ranges to view source ranges.
 */
class source_map{
public:
    using line_type     = std::size_t;
    using interval_type = boost::icl::interval<line_type>::type;
    using map_type      = boost::icl::interval_map<line_type, source_span>;

    inline void add(line_type lua_begin, line_type lua_end, source_span view_lines){
        if(lua_begin == 0 || lua_end <= lua_begin){
            throw std::invalid_argument{"generated Lua source interval must be non-empty"};
        }
        if(!view_lines.valid()){
            throw std::invalid_argument{"view source interval must be valid"};
        }

        interval_type generated = interval_type::right_open(lua_begin, lua_end);
        if(boost::icl::intersects(_lines, generated)){
            throw std::logic_error{"generated Lua source intervals must not overlap"};
        }

        _lines += std::make_pair(generated, view_lines);
    }

    inline std::optional<source_span> find(line_type lua_line) const {
        auto found = _lines.find(lua_line);
        if(found == _lines.end()){
            return std::nullopt;
        }
        return found->second;
    }

    inline bool empty() const noexcept { return _lines.empty(); }

private:
    map_type _lines;
};

}
}
}
}

#endif // UDHO_VIEW_BRIDGES_SOURCE_MAP_H
