#ifndef UDHO_EXAMPLES_SIMPLE_APPS_USER_H
#define UDHO_EXAMPLES_SIMPLE_APPS_USER_H

#include <udho/core/application.h>
#include <udho/net/stream.h>
#include <udho/hazo/string/basic.h>
#include <udho/url/operators.h>

namespace simple{
namespace apps{

/**
 * @todo write docs
 */
struct user: udho::core::application<apps::user>{
    auto routes() {
        using namespace udho::hazo::string::literals;
        return
            udho::url::slot("home"_h,    &user::home,    this) << udho::url::home(udho::url::verb::get)                 |
            udho::url::slot("profile"_h, &user::profile, this) << udho::url::scan(udho::url::verb::get, "/profile/{}")
        ;
    }
    void home(udho::net::stream context);
    void profile(udho::net::stream context, std::size_t id);
};

}
}

#endif // UDHO_EXAMPLES_SIMPLE_APPS_USER_H
