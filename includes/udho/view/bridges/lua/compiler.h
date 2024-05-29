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

#ifndef UDHO_VIEW_BRIDGES_LUA_COMPILER_H
#define UDHO_VIEW_BRIDGES_LUA_COMPILER_H

#include <string>
#include <vector>
#include <functional>
#include <sol/sol.hpp>
#include <udho/url/detail/format.h>
#include <udho/view/sections.h>
#include <udho/view/bridges/lua/fwd.h>
#include <udho/view/bridges/lua/script.h>
#include <udho/view/bridges/lua/state.h>

namespace udho{
namespace view{
namespace data{
namespace bridges{

namespace detail{
namespace lua{

struct compiler{
    using script_type = lua::script;

    compiler(detail::lua::state& state): _state(state) {}

    inline bool operator()(script_type&& script);

    private:
        state& _state;
};

bool compiler::operator()(script_type&& script){
    sol::load_result load_result = _state._state.load_buffer(script.data(), script.size());
    if (!load_result.valid()) {
        sol::error err = load_result;
        throw std::runtime_error("Error loading script: " + std::string(err.what()));
    }

    sol::protected_function view =  load_result.get<sol::protected_function>();
    sol::protected_function_result view_result = view();
    if (!view_result.valid()) {
        sol::error err = view_result;
        throw std::runtime_error("Error during function extraction: " + std::string(err.what()));
    }

    sol::protected_function view_fnc = view_result;
    auto it = _state._views.insert(std::make_pair(script.name(), view_fnc));
    return it.second;
}

}
}

}
}
}
}

#endif // UDHO_VIEW_BRIDGES_LUA_COMPILER_H
