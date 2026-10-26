#ifndef UDHO_VIEW_BRIDGES_LUA_DIAGNOSTIC_H
#define UDHO_VIEW_BRIDGES_LUA_DIAGNOSTIC_H


#include <sol/sol.hpp>

namespace udho{
namespace view{
namespace data{
namespace bridges{

namespace detail{
namespace lua{


struct diagnostic{
    std::string message;
    std::string traceback;
    std::string source;
    std::size_t line = 0;

    diagnostic() = default;
    diagnostic(const diagnostic&) = default;
    diagnostic(diagnostic&&) = default;

    inline explicit diagnostic(const sol::protected_function_result& result){
        extract(result);
    }

    inline diagnostic& operator=(const sol::protected_function_result& result) {
        clear();
        extract(result);
        return *this;
    }

    static inline sol::protected_function_result install(sol::state& state, const std::string& name) {
        static const std::string code = R"(
        local function udho_diagnostic_handler(error_value, expected_source)
            local generated_line   = 0
            local generated_source = ""
            local level = 2

            while true do
                local info = debug.getinfo(level, "Sl")
                if not info then break end
                if info.currentline
                    and info.currentline > 0
                    and (expected_source == nil or info.source == expected_source)
                then
                    generated_line = info.currentline
                    generated_source = info.source or ""
                    break
                end
                level = level + 1
            end

            return {
                message   = tostring(error_value),
                traceback = debug.traceback("", 2),
                source    = generated_source,
                line      = generated_line,
                error     = error_value
            }
        end

        local function udho_diagnostic_factory(expected_source)
            return function(error_value)
                return udho_diagnostic_handler(error_value, expected_source)
            end
        end

        return {
            handler = udho_diagnostic_handler,
            factory = udho_diagnostic_factory
        }
        )";

        return state.safe_script(code, sol::script_pass_on_error, name);
    }

private:
    void clear(){
        message.clear();
        traceback.clear();
        source.clear();
        line = 0;
    }

    void extract(const sol::protected_function_result& result){
        assert(!result.valid());
        sol::stack_object error_object{result.lua_state(), result.stack_index()};
        if(error_object.is<sol::table>()){
            sol::table error = error_object.as<sol::table>();
            message    = error.get_or<std::string>("message", "Unknown Lua error");
            traceback  = error.get_or<std::string>("traceback", "");
            source     = error.get_or<std::string>("source", "");
            line       = error.get_or<std::size_t>("line", 0);
        } else if(error_object.is<std::string>()){
            message = error_object.as<std::string>();
        } else {
            message = "Unknown Lua error";
        }
    }
};

}
}

}
}
}
}

#endif // UDHO_VIEW_BRIDGES_LUA_DIAGNOSTIC_H
