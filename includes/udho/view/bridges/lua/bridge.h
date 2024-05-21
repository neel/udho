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

#include <string>
#include <vector>
#include <functional>
#include <sol/sol.hpp>
#include <udho/url/detail/format.h>
#include <udho/view/sections.h>
#include <udho/view/bridges/lua/script.h>
#include <udho/view/bridges/lua/buffer.h>
#include <udho/view/bridges/lua/binder.h>

namespace udho{
namespace view{
namespace data{
namespace bridges{

namespace detail{
namespace lua{

struct bridge{
    template <typename X>
    using binder_type = binder<X>;
    using script_type = script;
    using buffer_type = buffer;

    inline bridge() {
        _state.open_libraries(sol::lib::base, sol::lib::string, sol::lib::math, sol::lib::utf8);
    }
    inline void init(){
        _udho = _state["udho"].get_or_create<sol::table>();
        buffer::apply(_udho);

        std::string script = R"(
            if jit then
                print("LuaJIT is being used")
                print("LuaJIT version: " .. jit.version)
            else
                print("LuaJIT is not being used")
            end
        )";

        _state.script(script);
    }

    template <typename ClassT, typename... Xs>
    void bind(udho::view::data::metatype<udho::view::data::associative<Xs...>>& type){
        bind<ClassT>(type.name(), type.members());
    }

    template <typename ClassT>
    void bind(udho::view::data::type<ClassT> handle){
        auto meta = prototype(handle);
        bind<ClassT>(meta);
    }

    /**
     * Creates an empty lua script. Reference to that script is supposed to be passed to the parser
     * @code
     * udho::view::data::bridges::lua::script_type script = lua.script("script.lua");
     * udho::view::sections::parser parser;
     * parser.parse(buffer, buffer+sizeof(buffer), script);
     * @endcode
     * It's the way of begining registration of a view.
     */
    script_type create(const std::string& name){ return script_type{name}; }

    /**
     * Compiles a lua_script. Assumes that the script is already parsed. Stores the lua function inside a map.
     */
    bool compile(script_type& script){
        sol::load_result load_result = _state.load_buffer(script.data(), script.size());
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
        auto it = _views.insert(std::make_pair(script.name(), view_fnc));
        return it.second;
    }

    /**
     * Executes the lua function from the map by the script name.
     */
    template <typename T>
    std::size_t exec(const std::string& name, const T& data, std::string& output){
        if(!_views.count(name)){
            std::cout << "View not found " << name << std::endl;
            return 0;
        }

        sol::protected_function view = _views[name];
        buffer_type buff;
        sol::protected_function_result result = view(data, buff);

        if (!result.valid()) {
            sol::error err = result;
            std::cout << "Error executing function from " << name << ": " << err.what() << std::endl;
            return 0;
        }

        std::size_t size = buff.str(output);
        return size;
    }

    inline void shell(){
        std::string line;
        std::cout << "Enter Lua commands or 'exit' to quit." << std::endl;
        while (true) {
            std::cout << "> ";
            std::getline(std::cin, line);

            if (line == "exit") {
                break;
            }

            try {
                sol::protected_function_result result = _state.script(line, sol::script_default_on_error);
                if (!result.valid()) {
                    sol::error err = result;
                    std::cerr << "Error executing Lua script: " << err.what() << std::endl;
                }
            } catch (const sol::error& err) {
                std::cerr << "Exception: " << err.what() << std::endl;
            }
        }
    }

    private:
        template <typename ClassT, typename... Xs>
        void bind(const std::string& name, udho::view::data::associative<Xs...>& assoc){
            binder_type<ClassT> user_type(_udho, name);
            assoc.apply(std::move(user_type));
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


