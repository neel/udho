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
#include <udho/hazo/string/basic.h>
#include <udho/url/detail/format.h>
#include <udho/view/bridges/lua/fwd.h>
#include <udho/view/bridges/lua/buffer.h>

namespace udho{
namespace view{
namespace data{
namespace bridges{

namespace detail{
namespace lua{

/**
 * @struct state
 * @brief Manages the Lua scripting environment, including libraries, global variables, and script execution.
 *
 * This structure encapsulates the Lua state management using the Sol2 library, providing functionalities for opening libraries, initializing global tables, and executing Lua scripts. It is designed to interact closely with Lua scripts generated from templates, facilitating data binding and script execution within a robust error handling framework.
 */
struct state{
    struct view_info{
        std::size_t min_buffer_size;
        sol::protected_function function;
    };

    friend compiler; ///< Allows compiler direct access to internal details for script compilation.

    template <typename X>
    friend struct binder;  ///< Allows binder template to access and modify Lua state for type bindings.

    using buffer_type   = buffer; ///< Alias for the buffer type used to handle script outputs.

    /**
     * @brief Constructs the Lua state and opens necessary Lua libraries.
     *
     * Opens essential libraries such as base, string, math, and utf8 to provide a rich standard environment for executing scripts.
     */
    inline state() {
        _state.open_libraries(sol::lib::base, sol::lib::string, sol::lib::math, sol::lib::utf8, sol::lib::debug);
    }

    static constexpr auto name() {
        using namespace udho::hazo::string::literals;
        return "lua"_h;
    }

    /**
     * @brief Initializes the Lua environment, setting up necessary tables and applying buffer bindings.
     *
     * Creates a 'udho' table in the Lua global environment if it does not exist and binds the buffer class to it. This table acts as a namespace for all operations related to the framework.
     */
    inline void init(){
        // _state.set_exception_handler([](lua_State* L, sol::optional<const std::exception&> maybe_exception, sol::string_view description) -> int {
        //     if (maybe_exception) {
        //         std::string message = "An exception occurred: ";
        //         message += maybe_exception->what();  // Get the exception message
        //         sol::stack::push(L, message);  // Push the message onto the Lua stack
        //     } else {
        //         // Push the description if no standard exception is caught
        //         sol::stack::push(L, std::string(description.data(), description.size()));
        //     }
        //     return 1;  // Return the number of values pushed onto the stack
        // });
        // https://snipplr.com/view/13085
        std::string dir_code = R"(
            function dir(obj, level)
                local s, t = '', type(obj)
                level = level or ''

                if t == 'nil' or t == 'boolean' or t == 'number' or t == 'string' then
                    s = tostring(obj)
                    if t == 'string' then
                        s = '"' .. s .. '"'
                    end
                elseif t == 'function' then
                    s = 'function'
                elseif t == 'userdata' or t == 'thread' then
                    local mt = getmetatable(obj)
                    if mt then
                        s = s .. '{\n'
                        for k, v in pairs(mt) do
                            local funcType = type(v) == 'function' and 'function' or 'value'
                            -- Heuristic to differentiate potential properties (common in userdata)
                            if type(v) == 'function' and (k:sub(1, 3) == 'get' or k:sub(1, 3) == 'set') then
                                funcType = 'property'
                            end
                            s = s .. level .. '  ' .. tostring(k) .. ': ' .. funcType .. ',\n'
                        end
                        s = s .. level .. '}'
                    else
                        s = tostring(obj) -- Default to a simple tostring if no metatable or special handling defined
                    end
                elseif t == 'table' then
                    s = '{\n'
                    for k, v in pairs(obj) do
                        local k_str = tostring(k)
                        if type(k) == 'string' then
                            k_str = '["' .. k_str .. '"]'
                        end
                        s = s .. level .. '  ' .. k_str .. ' = ' .. dir(v, level .. '  ') .. ',\n'
                    end
                    s = s .. level .. '}'
                end

                return s
            end
        )";
        _state.script(dir_code);
        _udho = _state["udho"].get_or_create<sol::table>();
        buffer::apply(_udho);
    }


    /**
     * @brief Executes a Lua script identified by its name, using provided data and capturing the output.
     *
     * @param name The identifier of the script to execute.
     * @param data The data to be passed to the script, typically involving context or configuration.
     * @param output Reference to a string where the script's output will be stored.
     * @return The size of the generated output.
     * @tparam T The type of the data passed to the script.
     *
     * Searches for the script in the internal map and executes it if found. If the script execution is successful, captures the output using the provided buffer. Handles and reports errors if the script execution fails.
     */
    template <typename T, typename Aux>
    std::size_t exec(const std::string& name, const T& data, const Aux& aux, std::string& output){
        std::string view_index = name;
        if(!_views.count(view_index)){
            std::cout << "View not found " << view_index << std::endl;
            return 0;
        }

        view_info view = _views[view_index];
        buffer_type buff{view.min_buffer_size};
        sol::protected_function_result result = view.function(std::ref(data), std::ref(aux), buff);

        if (!result.valid()) {
            sol::error err = result;
            std::cout << "Error executing function from " << view_index << ": " << err.what() << std::endl;
            return 0;
        }

        std::size_t size = buff.str(output);
        return size;
    }

    sol::table& udho() { return _udho; }

    private:
        sol::state _state; ///< The underlying Sol2 state object managing the Lua environment.
        sol::table _udho; ///< A table in the Lua global environment for namespaced operations related to this framework.
        std::map<std::string, view_info> _views;
};

}
}

}
}
}
}

#endif // UDHO_VIEW_BRIDGES_LUA_BRIDGE_H


