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

#ifndef UDHO_VIEW_BRIDGES_LUA_H
#define UDHO_VIEW_BRIDGES_LUA_H

#include <udho/view/bridges/lua/state.h>
#include <udho/view/bridges/lua/compiler.h>
#include <udho/view/bridges/lua/script.h>
#include <udho/view/bridges/bridge.h>
#include <fmt/core.h>
#include <fmt/args.h>

namespace udho{
namespace view{
namespace data{
namespace bridges{

/**
 * @typedef lua
 * @brief A specialized bridge configured for Lua scripting.
 *
 * This type alias represents a specific instantiation of the `bridge` template, configured to use Lua-specific components for state management, scripting, and binding. It encapsulates the interaction between template parsing, Lua script generation, and execution, providing a streamlined interface for integrating Lua scripting into the template engine.
 *
 * @details The lua type is defined using several Lua-specific components:
 * - `detail::lua::state`: Manages the state specific to Lua scripts, such as global variables and function registrations.
 * - `detail::lua::compiler`: Responsible for compiling Lua scripts into a form that can be executed by the Lua interpreter.
 * - `detail::lua::script`: Handles the generation of Lua scripts from parsed template sections.
 * - `detail::lua::binder`: Provides mechanisms to bind C++ data structures to Lua scripts, enabling data exchange between C++ and Lua.
 */
using lua = udho::view::data::bridges::bridge<
                detail::lua::state,
                detail::lua::compiler,
                detail::lua::script,
                detail::lua::binder
            >;

}
}
}
}

namespace udho::view::data{

    template <>
    struct bind<bridges::lua, udho::url::summary::mount_point::url_proxy>{

        using state_type  = typename bridges::lua::state_type;
        using class_type  = udho::url::summary::mount_point::url_proxy;
        using binder_type = typename bridges::lua::template default_binder_type<class_type>;

        static void apply(state_type& state){
            using user_type = sol::usertype<class_type>;

            std::cout << "udho::view::data::bind<lua, url_proxy>: binding" << std::endl;

            user_type type = state.udho().new_usertype<class_type>("url_proxy",
                "new", sol::no_constructor,
                "pattern", sol::property(&class_type::pattern)
            );

            type.set_function("replace", [](const class_type& self, sol::variadic_args va) {
                return replacement(self, va);
            });

            type.set_function("_call", [](const class_type& self, sol::variadic_args va) {
                return replacement(self, va);
            });
        }

        private:
            static std::string replacement(const class_type& self, sol::variadic_args va){
                fmt::dynamic_format_arg_store<fmt::format_context> store;
                for (auto v : va) {
                    if (v.is<int>()) {
                        store.push_back(v.as<int>());
                    } else if (v.is<double>()) {
                        store.push_back(v.as<double>());
                    } else if (v.is<bool>()) {
                        store.push_back(v.as<bool>());
                    } else {
                        store.push_back(v.as<std::string>());
                    }
                }
                return fmt::vformat(self.pattern(), store);
            }
    };
}


#endif // UDHO_VIEW_BRIDGES_LUA_H

