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

#ifndef UDHO_VIEW_BRIDGES_LUA_BUFFER_H
#define UDHO_VIEW_BRIDGES_LUA_BUFFER_H

#include <string>
#include <vector>
#include <functional>
#include <sol/sol.hpp>

namespace udho{
namespace view{
namespace data{
namespace bridges{

namespace detail{
namespace lua{

struct buffer{
    inline explicit buffer(std::size_t min_size = 0) {
        if(min_size > 0){
            _buffer.reserve(min_size);
        }
    }

    inline std::size_t write(const sol::string_view& lua_string) {
        _buffer.append(lua_string.data(), lua_string.size());
        std::size_t size = lua_string.size();
        return size;
    }


    inline void print(sol::variadic_args va, sol::this_state state) {
        sol::state_view lua(state);

        std::string result;
        bool first = true;
        for (const auto& v : va) {
            if (!first) {
                result.append("\t");
            }
            first = false;

            switch (v.get_type()) {
                case sol::type::number:
                    if (v.is<int>()) {
                        result.append(std::to_string(v.as<int>()));
                    } else {
                        result.append(std::to_string(v.as<double>()));
                    }
                    break;
                case sol::type::boolean:
                    result.append(v.as<bool>() ? "true" : "false");
                    break;
                case sol::type::string:
                    result.append(v.as<std::string>());
                    break;
                default:
                    sol::function tostring = lua["tostring"];
                    result.append(tostring(v));
                    break;
            }
        }
        result.append("\n");
        _buffer.append(result);
    }

    inline std::size_t str(std::string& result) const {
        result = _buffer;
        return _buffer.size();
    }

    inline std::size_t size() const { return _buffer.size(); }
    inline void clear() { _buffer.clear(); }

    inline static sol::usertype<buffer> apply(sol::table& table, const std::string& name = "udho_buffer"){
        return table.new_usertype<buffer>(name,
            "write", &buffer::write,
            "size",  &buffer::size,
            "clear", &buffer::clear,
            "print", &buffer::print
        );
    }

    private:
        std::string _buffer;
};

}
}

}
}
}
}

#endif // UDHO_VIEW_BRIDGES_LUA_BUFFER_H
