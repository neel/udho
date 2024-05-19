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
#include <vector>
#include <functional>
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

struct lua_string_buffer{
    inline explicit lua_string_buffer(): _len(0) {}

    inline std::size_t write(const sol::string_view& lua_string) {
        _buffer.emplace_back(lua_string);
        std::size_t size = lua_string.size();
        _len += size;
        return size;
    }

    inline std::size_t str(std::string& result) const {
        result.clear();
        result.reserve(_len);

        for (const sol::string_view& view : _buffer) {
            result.append(view.data(), view.size());
        }

        return _len;
    }

    inline std::size_t size() const { return _len; }
    inline void clear() { _buffer.clear(); }

    inline static sol::usertype<lua_string_buffer> apply(sol::table& table, const std::string& name = "lua_string_buffer"){
        return table.new_usertype<lua_string_buffer>(name,
            "write", &lua_string_buffer::write,
            "size",  &lua_string_buffer::size,
            "clear", &lua_string_buffer::clear
        );
    }

    private:
        std::vector<sol::string_view> _buffer;
        std::size_t _len;
};

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
    inline explicit lua_script(const std::string& name): _name(name) {
        append("return function(d, stream)");
        append_newline();
    }
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
    std::string name() const { return _name; }
    std::string body() const {
        return std::string(_buffer.begin(), _buffer.end());
    }
    const char* data() const { return _buffer.data(); }
    std::size_t size() const { return _buffer.size(); }
    void finish(){
        append("end");
    }
    private:
        inline void add_eval_section(const udho::view::sections::section& section) {
            const std::string& content = section.content();
            _buffer.insert(_buffer.end(), content.begin(), content.end());
        }

        inline void add_echo_section(const udho::view::sections::section& section) {
            if (section.size() > 0) {
                append_newline();
                append("do -- " + udho::url::format("{}", udho::view::sections::section::name(section.type())) + "\n");
                if (section.type() == udho::view::sections::section::echo) {
                    append("\t" + udho::url::format("local udho_view_str_ = string.format([=====[%s]=====], {})", section.content()) + "\n");
                } else {
                    append("\t" + udho::url::format("local udho_view_str_ = [=====[{}]=====]", section.content()) + "\n");
                }
                append("\tstream:write(udho_view_str_)\n");
                append("end\n");
            }
        }

        inline void discard_section(const udho::view::sections::section&){}
    private:
        void append(const std::string& str) { _buffer.insert(_buffer.end(), str.begin(), str.end()); }
        void append_newline() { _buffer.push_back('\n'); }
    private:
        std::string _name;
        std::vector<char> _buffer;
};

struct lua{
    template <typename X>
    using binder = lua_binder<X>;

    inline lua() {
        _state.open_libraries(sol::lib::base, sol::lib::string, sol::lib::math, sol::lib::utf8);
    }
    inline void init(){
        _udho = _state["udho"].get_or_create<sol::table>();
        lua_string_buffer::apply(_udho);
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
     * udho::view::data::bridges::lua_script script = lua.script("script.lua");
     * udho::view::sections::parser parser;
     * parser.parse(buffer, buffer+sizeof(buffer), script);
     * @endcode
     * It's the way of begining registration of a view.
     */
    lua_script script(const std::string& name){ return lua_script{name}; }

    /**
     * Compiles a lua_script. Assumes that the script is already parsed. Stores the lua function inside a map.
     */
    bool compile(lua_script& script){
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
    std::string exec(const std::string& name, const T& data){
        if(!_views.count(name)){
            std::cout << "View not found " << name << std::endl;
            return std::string{};
        }

        sol::protected_function view = _views[name];
        lua_string_buffer buffer;
        sol::protected_function_result result = view(data, buffer);

        if (!result.valid()) {
            sol::error err = result;
            std::cout << "Error executing function from " << name << ": " << err.what() << std::endl;
            return std::string{};
        }

        std::string output;
        std::size_t size = buffer.str(output);
        return output;
    }

    inline void shell();

    private:
        template <typename ClassT, typename... Xs>
        void bind(const std::string& name, udho::view::data::associative<Xs...>& assoc){
            binder<ClassT> user_type(_udho, name);
            assoc.apply(std::move(user_type));
        }

    private:
        sol::state _state;
        sol::table _udho;
        std::map<std::string, sol::protected_function> _views;
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

