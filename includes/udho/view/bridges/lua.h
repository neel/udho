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

#include <string>
#include <sstream>
#include <nlohmann/json.hpp>
#include <sol/sol.hpp>
#include <udho/view/scope.h>
#include <udho/view/sections.h>
#include <udho/url/detail/format.h>

namespace udho{
namespace view{
namespace data{
namespace bridges{

template <typename X>
struct lua_binder{
    using user_type = sol::usertype<X>;

    lua_binder(sol::table& table, const std::string& name): _type(table.new_usertype<X>(name)) {}

    template <typename KeyT, typename T>
    lua_binder& operator()(udho::view::data::nvp<udho::view::data::policies::property<udho::view::data::policies::writable>, KeyT, udho::view::data::wrapper<T>>& nvp){
        auto& w = nvp.wrapper();
        _type.set(nvp.name(), *w);
        return *this;
    }
    template <typename KeyT, typename T>
    lua_binder& operator()(udho::view::data::nvp<udho::view::data::policies::property<udho::view::data::policies::readonly>, KeyT, udho::view::data::wrapper<T>>& nvp){
        auto& w = nvp.wrapper();
        _type.set(nvp.name(), sol::readonly(*w));
        return *this;
    }
    template <typename KeyT, typename U, typename V>
    lua_binder& operator()(udho::view::data::nvp<udho::view::data::policies::property<udho::view::data::policies::functional>, KeyT, udho::view::data::wrapper<U, V>>& nvp){
        auto& w = nvp.wrapper();
        _type.set(nvp.name(), sol::property(
            *static_cast<udho::view::data::getter_value<U>&>(w),
            *static_cast<udho::view::data::setter_value<V>&>(w)
        ));
        return *this;
    }
    template <typename KeyT, typename T>
    lua_binder& operator()(udho::view::data::nvp<udho::view::data::policies::function, KeyT, udho::view::data::wrapper<T>>& nvp){
        auto& w = nvp.wrapper();
        _type.set(nvp.name(), *w);
        return *this;
    }
    private:
        user_type _type;
};

struct lua_script{
    inline explicit lua_script(const std::string& name): _name(name) {}
    inline void add_section(const udho::view::sections::section& section){
        switch(section.type()){
            case udho::view::sections::section::text:
            case udho::view::sections::section::verbatim:
            case udho::view::sections::section::echo:
                add_echo_section(section);
                break;
            case udho::view::sections::section::eval:
                add_eval_section(section);
                break;
            default:
                break;
        }
    }
    inline void operator()(const udho::view::sections::section& section){
        add_section(section);
    }
    std::string body() const {
        return _stream.str();
    }
    private:
        inline void add_eval_section(const udho::view::sections::section& section){
            _stream << section.content();
        }
        inline void add_echo_section(const udho::view::sections::section& section){
            _stream << std::endl;
            _stream << "do -- " << udho::url::format("{}/{}", udho::view::sections::section::name(section.type()), section.id()) << std::endl;
            _stream << "\t" << udho::url::format("local ustr_{} = [=====[", section.id()) << section.content() << "]=====]" << std::endl;
            _stream << "\t" << udho::url::format("print(ustr_{})", section.id()) << std::endl;
            _stream << "end" << std::endl;
        }
        inline void discard_section(const udho::view::sections::section&){}
    private:
        std::string _name;
        std::stringstream _stream;
};

struct lua{
    template <typename X>
    using binder = lua_binder<X>;

    inline lua() {
        _state.open_libraries(sol::lib::base);
    }
    inline void init(){
        _udho = _state["udho"].get_or_create<sol::table>();
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

    lua_script script(const std::string& name){
        return lua_script{name};
    }

    inline void shell();

    private:
        template <typename ClassT, typename... Xs>
        void bind(const std::string& name, udho::view::data::associative<Xs...>& assoc){
            binder<ClassT> user_type(_udho, name);
            assoc.apply(std::move(user_type));
            _state.script(R"(
                print("begin lua")
                print("Inspecting table 'udho':")
                for key, value in pairs(udho) do
                    print(key, type(value))
                end
                local obj = udho.info.new()
                print(obj.name)
                print(obj.value)

                obj.name = "changed"
                obj:print()
                print("end lua")
            )");
        }

    private:
        sol::state _state;
        sol::table _udho;
};

void lua::shell(){
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


}
}
}
}


#endif // UDHO_VIEW_BRIDGES_LUA_H

