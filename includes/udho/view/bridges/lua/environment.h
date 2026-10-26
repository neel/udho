#ifndef UDHO_VIEW_BRIDGES_LUA_ENVIRONMENT_H
#define UDHO_VIEW_BRIDGES_LUA_ENVIRONMENT_H

#include <sol/sol.hpp>

namespace udho{
namespace view{
namespace data{
namespace bridges{

namespace detail{
namespace lua{

struct environment_guard{
    environment_guard() = delete;
    environment_guard(const environment_guard&) = delete;
    environment_guard(environment_guard&&) = delete;

    inline environment_guard(sol::state& lua, sol::protected_function& function): _function(function), _previous(sol::get_environment(function)), _isolated(lua, sol::create, lua.globals()) {
        sol::set_environment(_isolated, _function);
    }

    ~environment_guard() noexcept {
        sol::set_environment(_previous, _function);
    }

    sol::protected_function& _function;
    sol::environment _previous;
    sol::environment _isolated;
};

} // namespace lua
}
}
}
}
}

#endif // UDHO_VIEW_BRIDGES_LUA_ENVIRONMENT_H
