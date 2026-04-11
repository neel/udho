#ifndef UDHO_VIEW_BRIDGES_LUA_EXTRAS_H
#define UDHO_VIEW_BRIDGES_LUA_EXTRAS_H

#include <chrono>
#include <thread>
#include <sol/sol.hpp>
#include <udho/utils/encoding.h>

namespace udho{
namespace view{
namespace data{
namespace bridges{

namespace detail{
namespace lua{

struct extras{
    explicit extras(sol::table& utils): _utils(utils) {}

    void operator()() {
        _utils.set_function("sleep", [](std::size_t millisecs){
            std::this_thread::sleep_for(std::chrono::milliseconds(millisecs));
        });

        _utils.set_function("thread_id", []() -> std::thread::id {
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

        _utils.set_function("htmlescape", [](std::string input){
            return udho::utils::encode::escape(input);
        });
    }

private:
    sol::table& _utils;
};

}
}

}
}
}
}

#endif // UDHO_VIEW_BRIDGES_LUA_EXTRAS_H
