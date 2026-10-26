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
#include <exception>
#include <sstream>
#include <utility>
#include <sol/sol.hpp>
#include <udho/hazo/string/basic.h>
#include <udho/url/detail/format.h>
#include <udho/view/bridges/lua/fwd.h>
#include <udho/view/bridges/lua/buffer.h>
#include <udho/view/bridges/lua/extras.h>
#include <udho/view/bridges/exceptions.h>
#include <udho/view/bridges/source_map.h>
#include <udho/manifold/context.h>
#include <udho/logging/macros.h>
#include <udho/view/bridges/lua/environment.h>
#include <udho/view/bridges/lua/diagnostic.h>
#include <udho/view/resources/resource.h>

namespace udho{
namespace view{
namespace data{
namespace bridges{

namespace detail{
namespace lua{

/**
 * @addtogroup DoxyG_view_tmpl_lua
 * @{
 */

namespace scripts{

inline const std::string& dir() {
    // https://snipplr.com/view/13085
    static const std::string code = R"(
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

    return code;
}


}
/**
 * @struct state
 * @brief Manages the Lua scripting environment, including libraries, global variables, and script execution.
 *
 * This structure encapsulates the Lua state management using the Sol2 library, providing functionalities for opening libraries, initializing global tables, and executing Lua scripts. It is designed to interact closely with Lua scripts generated from templates, facilitating data binding and script execution within a robust error handling framework.
 */
struct state{
    using view_resource_type      = udho::view::resources::tmpl::resource;
    using view_resource_cref_type = std::reference_wrapper<const view_resource_type>;

    inline static std::uint32_t generate_id(){
        static std::uint32_t id = 0;
        return id++;
    }

    struct view_info{
        view_info(std::size_t size, sol::protected_function fn, bridges::source_map map, view_resource_cref_type resource)
            : min_buffer_size(size), function(std::move(fn)), source_map(std::move(map)), resource(resource) {}

        std::size_t min_buffer_size;
        sol::protected_function function;
        bridges::source_map source_map;
        view_resource_cref_type resource;
    };

    using id_type = std::uint32_t;

    friend compiler; ///< Allows compiler direct access to internal details for script compilation.

    template <typename X>
    friend struct binder;  ///< Allows binder template to access and modify Lua state for type bindings.

    using buffer_type   = buffer; ///< Alias for the buffer type used to handle script outputs.

    /**
     * @brief Constructs the Lua state and opens necessary Lua libraries.
     *
     * Opens essential libraries such as base, string, math, and utf8 to provide a rich standard environment for executing scripts.
     */
    inline state(): _id(state::generate_id()) {
        _state.open_libraries(sol::lib::base, sol::lib::string, sol::lib::math, sol::lib::utf8, sol::lib::debug);
        _lua_version = {0, 0};
    }

    id_type id() const { return _id; }

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
        namespace p = udho::logging::params;

        // { error handling
        _state.set_exception_handler(&state::exception_handler);
        sol::protected_function_result handler_result = udho::view::data::bridges::detail::lua::diagnostic::install(_state, "@udho/view/error-handler");
        if(!handler_result.valid()){
            sol::error err = handler_result;
            throw std::runtime_error{"Failed to initialize Lua view error handler: " + std::string(err.what())};
        }
        if(handler_result.get_type() != sol::type::table){
            throw std::runtime_error{"Failed to initialize Lua view error handler: initialization did not return a table"};
        }

        sol::table error_handler_table = handler_result.get<sol::table>();
        sol::object diagnostic_handler = error_handler_table["handler"];
        sol::object diagnostic_factory = error_handler_table["factory"];
        if(!diagnostic_handler.is<sol::function>() || !diagnostic_factory.is<sol::function>()){
            throw std::runtime_error{"Failed to initialize Lua view error handler: invalid handler functions"};
        }

        _diagnostic_handler = diagnostic_handler.as<sol::function>();
        sol::protected_function::set_default_handler(_diagnostic_handler);
        _diagnostic_factory = diagnostic_factory.as<sol::protected_function>();
        // }

        // { version check
        try {
            // Execute the script and directly obtain the version string
            auto result = _state.safe_script("return _VERSION", sol::script_pass_on_error);
            if (!result.valid()) {
                udho::view::data::bridges::detail::lua::diagnostic error(result);
                // std::cerr << "Failed to detect lua version: " << err.what() << std::endl;
                UDHO_LOG_ERROR("udho::view::data::bridges::lua::state", "Failed to detect Lua version", p::exception(error.message));
                return;
            }

            std::string lua_version_str = result.get<std::string>();
            _lua_version = parse_lua_version(lua_version_str);
            // std::cout << "Detected Lua version: " << _lua_version.first << "." << _lua_version.second << std::endl;
            UDHO_LOG_DEBUG("udho::view::data::bridges::lua::state", "Detected Lua version " + std::to_string(_lua_version.first) + "." + std::to_string(_lua_version.second));
        } catch (const sol::error& e) {
            // std::cerr << "Error while detecting lua version: " << e.what() << std::endl;
            UDHO_LOG_ERROR("udho::view::data::bridges::lua::state", "Error while detecting Lua version", p::exception(e.what()));
            return;
        }
        // }

        // { auxilary lua functions
        sol::protected_function_result dir_result = _state.safe_script(scripts::dir(), sol::script_pass_on_error, "@udho/view/dir");
        if(!dir_result.valid()){
            udho::view::data::bridges::detail::lua::diagnostic error(dir_result);
            throw std::runtime_error{"Failed to initialize Lua dir function: " + std::move(error.message)};
        }

        _udho = _state["udho"].get_or_create<sol::table>();
        buffer::apply(_udho);

        _utils  = _udho["utils"].get_or_create<sol::table>();
        udho::view::data::bridges::detail::lua::extras extras(_utils);
        extras();
        // }
    }


    /**
     * @brief Executes a Lua script identified by its name, using provided data and capturing the output.
     *
     * @param name The identifier of the script to execute.
     * @param data The data to be passed to the script, typically involving context or configuration.
     * @param aux Auxiliary data.
     * @param output Reference to a string where the script's output will be stored.
     * @return The size of the generated output.
     * @tparam T The type of the data passed to the script.
     *
     * Searches for the script in the internal map and executes it if found. If the script execution is successful, captures the output using the provided buffer. Handles and reports errors if the script execution fails.
     */
    template <typename T, typename Aux>
    std::size_t exec(const std::string& name, const T& data, Aux& aux, std::string& output){
        namespace p = udho::logging::params;

        output.clear();
        auto found = _views.find(name);
        if(found == _views.end()){
            UDHO_LOG_ERROR("udho::view::data::bridges::lua::state", "view not found", p::name(name));
            throw std::runtime_error{"View not found: " + name};
        }

        std::string rendered = execute(name, found->second, std::ref(data), std::ref(aux));
        output = std::move(rendered);
        return output.size();
    }

    /**
     * @brief Executes a Lua script identified by its name, using provided data and capturing the output.
     *
     * @warning supposed to be called from lua
     * @param name The identifier of the script to execute.
     * @param d The data to be passed to the script, typically involving context or configuration.
     * @param aux Auxiliary data.
     * @return The size of the generated output.
     * @tparam AuxT Auxiliary data type.
     *
     * Searches for the script in the internal map and executes it if found. If the script execution is successful, captures the output using the provided buffer. Handles and reports errors if the script execution fails.
     */
    template <typename AuxT>
    std::string exec_lua(const std::string& name, sol::object d, AuxT aux){
        namespace p = udho::logging::params;

        auto found = _views.find(name);
        if(found == _views.end()){
            UDHO_LOG_ERROR("udho::view::data::bridges::lua::state", "view not found", p::path(name));
            throw std::runtime_error{"View not found: " + name};
        }

        return execute(name, found->second, d, aux);
    }

    sol::table& udho() { return _udho; }
    const std::pair<int, int>& version() const { return _lua_version; }

    template <typename UnbinderF>
    void add_unbinder(UnbinderF&& unbinder) const {
        _unbinders.emplace_back(std::move(unbinder));
    }
    ~state(){
        for(auto& unbinder: _unbinders){
            unbinder(*this);
        }
    }
    private:
        struct cpp_exception{
            std::string message;
            std::exception_ptr exception;

            std::string to_string() const { return message; }
        };

        /**
         * @brief gets called by sol when the lua code in the view calls some C++ function that throws C++ exception.
         *        pushes that C++ exception to lua stack.
         * @param state
         * @param description
         * @return
         */
        static int exception_handler(lua_State* lua, sol::optional<const std::exception&>, sol::string_view description){
            cpp_exception value{std::string{description.data(), description.size()}, std::current_exception()};
            return sol::stack::push(lua, std::move(value));
        }

        /**
         * @brief returns diagnostic handler lua function for a view identified by the source.
         *        compiler sets that function as the error handler for the view.
         * @param source identifying the view
         * @return
         */
        sol::function diagnostic_handler(const std::string& source){
            sol::protected_function_result result = _diagnostic_factory(source);
            if(!result.valid()){
                udho::view::data::bridges::detail::lua::diagnostic error(result);
                throw std::runtime_error{"Error creating view diagnostic handler: " + std::move(error.message)};
            }
            if(result.get_type() != sol::type::function){
                throw std::runtime_error{"Error creating view diagnostic handler: factory did not return a function"};
            }
            return result.get<sol::function>();
        }

        static bridges::rendering_frame::source_lines_type extract_source_lines(const view_resource_type& resource, const bridges::source_span& span){
            bridges::rendering_frame::source_lines_type lines;
            if(!span.valid()){
                return lines;
            }

            auto line_begin = resource.begin();
            std::size_t line_number = 1;
            for(auto position = resource.begin(); position != resource.end(); ++position){
                if(*position != '\n'){
                    continue;
                }

                if(line_number >= span.begin && line_number <= span.end){
                    auto line_end = position;
                    if(line_end != line_begin && *(line_end - 1) == '\r'){
                        --line_end;
                    }
                    lines.push_back({line_number, std::string{line_begin, line_end}});
                }
                if(line_number == span.end){
                    return lines;
                }

                ++line_number;
                line_begin = position + 1;
            }

            if(line_number >= span.begin && line_number <= span.end){
                lines.push_back({line_number, std::string{line_begin, resource.end()}});
            }
            return lines;
        }

        template <typename... Args>
        std::string execute(const std::string& name, view_info& view, Args&&... args){
            namespace p = udho::logging::params;

            buffer_type buff{view.min_buffer_size};
            sol::protected_function_result result;
            std::exception_ptr exception;
            std::string message;
            try {
                environment_guard environment{_state, view.function};
                result = view.function(std::forward<Args>(args)..., buff);
            } catch(const std::exception& ex) {
                exception = std::current_exception();
                message = ex.what();
            } catch(...) {
                exception = std::current_exception();
                message = "A nested render or bound C++ operation failed";
            }

            std::string output;
            buff.str(output);
            if(!output.empty() && output.back() == '\0'){
                output.pop_back();
            }

            if(exception){
                bridges::rendering_frame frame{name, std::move(output)};
                throw bridges::rendering_error{std::move(frame), std::move(message), std::move(exception)};
            }

            if(!result.valid()){
                std::exception_ptr cause;
                sol::stack_object error_object{result.lua_state(), result.stack_index()};
                if(error_object.is<sol::table>()){
                    sol::table error_table = error_object.as<sol::table>();
                    sol::object error_value = error_table["error"];
                    if(error_value.is<cpp_exception>()){
                        cause = error_value.as<cpp_exception&>().exception;
                    }
                }

                udho::view::data::bridges::detail::lua::diagnostic error{result};
                std::optional<bridges::source_span> source = view.source_map.find(error.line);
                bridges::source_span lines = source ? *source : bridges::source_span{};
                bridges::rendering_frame::source_lines_type source_lines = extract_source_lines(view.resource.get(), lines);

                UDHO_LOG_ERROR("udho::view::data::bridges::lua::state", "view execution failed", p::exception(error.message), p::path(name));

                bridges::rendering_frame frame{name, std::move(output), error.line, lines, std::move(source_lines), std::move(error.traceback)};
                if(cause){
                    try {
                        std::rethrow_exception(cause);
                    } catch(const bridges::rendering_error& nested) {
                        throw bridges::rendering_error{std::move(frame), nested};
                    } catch(...) {
                    }
                }

                throw bridges::rendering_error{std::move(frame), std::move(error.message), std::move(cause)};
            }

            return output;
        }

        std::pair<int, int> parse_lua_version(const std::string& version_string) {
            std::istringstream iss(version_string);
            std::string token;
            std::getline(iss, token, ' ');  // Skip "Lua" prefix

            std::getline(iss, token, '.'); // Get major version part
            int major = std::stoi(token);
            std::getline(iss, token);      // Get minor version part, assuming no patch version handling
            int minor = std::stoi(token);

            return {major, minor};
        }

    private:
        sol::state _state; ///< The underlying Sol2 state object managing the Lua environment.
        sol::table _udho; ///< A table in the Lua global environment for namespaced operations related to this framework.
        std::map<std::string, view_info> _views;
        sol::table _utils;
        sol::function _diagnostic_handler;
        sol::protected_function _diagnostic_factory;
        std::pair<int, int> _lua_version;
        id_type _id;
        mutable std::vector<std::function<void (const state&)>> _unbinders;
};

/** @} */

} // namespace lua
}
}
}
}
}

#endif // UDHO_VIEW_BRIDGES_LUA_BRIDGE_H
