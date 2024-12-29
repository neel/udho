#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>
#include <thread>
#include <iostream>
#include <udho/view/tmpl/layout/property_tree.h>
#include <udho/view/tmpl/layout/placeholder.h>

int my_exception_handler(lua_State* L, sol::optional<const std::exception&> maybe_exception, sol::string_view description) {
	// L is the lua state, which you can wrap in a state_view if necessary
	// maybe_exception will contain exception, if it exists
	// description will either be the what() of the exception or a description saying that we hit the general-case catch(...)
	std::cout << "An exception occurred in a function, here's what it says ";
	if (maybe_exception) {
		std::cout << "(straight from the exception): ";
		const std::exception& ex = *maybe_exception;
		std::cout << ex.what() << std::endl;
	}
	else {
		std::cout << "(from the description parameter): ";
		std::cout.write(description.data(), static_cast<std::streamsize>(description.size()));
		std::cout << std::endl;
	}

	// you must push 1 element onto the stack to be
	// transported through as the error object in Lua
	// note that Lua -- and 99.5% of all Lua users and libraries -- expects a string
	// so we push a single string (in our case, the description of the error)
	return sol::stack::push(L, description);
}

int main(){

    namespace layout = udho::view::tmpl::layout;

    layout::menu menu{"root"};
    auto& child1 = menu.add("child1");
    child1.add("child1.1");
    child1.add("child1.2");
    auto& child2 = menu.add("child2");
    child2.add("child2.1");
    child2.add("child2.1");
    menu.write(std::cout, 0);

    layout::placeholders::standard p;
    std::cout << p[layout::placeholders::central].exists() << std::endl;
    std::cout << p[nullptr].exists() << std::endl;

    // auto value = menu.property("key");

    sol::state lua;
    lua.open_libraries(sol::lib::base, sol::lib::debug, sol::lib::string, sol::lib::math, sol::lib::utf8);
    lua.set_exception_handler(&my_exception_handler);
    lua.set_function("thread_id", []() -> std::thread::id {
        try{
            return std::this_thread::get_id();
        } catch(const std::exception& ex){
            std::cout << "Exception from lua calling thread.id: " << ex.what() << std::endl;
            return std::thread::id{};
        }catch (...) {
            std::cout << "An unknown exception occurred." << std::endl;
            return std::thread::id{};
        }
    });
    const char buffer[] = R"(
        return function()
            local function fn()
                print(thread_id())
            end
            local success, resultOrError = pcall(fn)
            if not success then
                print('Error executing fn: ' .. resultOrError)
            end
        end
    )";
    sol::load_result load_result = lua.load_buffer(buffer, strlen(buffer));
    if (!load_result.valid()) {
        sol::error err = load_result;
        std::cerr << "Failed to load buffer: " << err.what() << std::endl;
        return 1;
    }
    sol::protected_function fn =  load_result.get<sol::protected_function>();
    sol::protected_function returned_fn = fn();
    sol::protected_function_result result = returned_fn();
    std::cout << "After calling Lua function" << std::endl;
    if (!result.valid()) {
        sol::error err = result;
        std::cerr << "Failed to execute function: " << err.what() << std::endl;
        return 1;
    }
    return 0;
}
