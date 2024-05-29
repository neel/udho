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

#ifndef UDHO_VIEW_BRIDGES_LUA_BRIDGE_H
#define UDHO_VIEW_BRIDGES_LUA_BRIDGE_H

#include <map>
#include <string>
#include <functional>
#include <sol/sol.hpp>
#include <udho/url/detail/format.h>
#include <udho/view/bridges/lua/fwd.h>
#include <udho/view/bridges/lua/buffer.h>

namespace udho{
namespace view{
namespace data{
namespace bridges{

namespace detail{
namespace lua{

struct state{
    friend compiler;
    template <typename X>
    friend struct binder;

    using buffer_type   = buffer;

    inline state() {
        _state.open_libraries(sol::lib::base, sol::lib::string, sol::lib::math, sol::lib::utf8);
    }
    inline void init(){
        _udho = _state["udho"].get_or_create<sol::table>();
        buffer::apply(_udho);
    }

    template <typename T>
    std::size_t exec(const std::string& name, const T& data, std::string& output){
        std::string view_index = name;
        if(!_views.count(view_index)){
            std::cout << "View not found " << view_index << std::endl;
            return 0;
        }

        sol::protected_function view = _views[view_index];
        buffer_type buff;
        sol::protected_function_result result = view(data, buff);

        if (!result.valid()) {
            sol::error err = result;
            std::cout << "Error executing function from " << view_index << ": " << err.what() << std::endl;
            return 0;
        }

        std::size_t size = buff.str(output);
        return size;
    }

    private:
        sol::state _state;
        sol::table _udho;
        std::map<std::string, sol::protected_function> _views;
};

}
}

}
}
}
}

#endif // UDHO_VIEW_BRIDGES_LUA_BRIDGE_H


