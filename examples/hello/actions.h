// SPDX-License-Identifier: BSD-3-Clause

#ifndef UDHO_EXAMPLES_HELLO_ACTIONS_H
#define UDHO_EXAMPLES_HELLO_ACTIONS_H

#include <boost/config.hpp>
#include <string>
#include <udho/view/bridges/lua.h>
#include <udho/www/www.h>

namespace hello {
namespace actions {

using context = udho::www::context<udho::www::components::resources<udho::view::data::bridges::lua>>;

BOOST_SYMBOL_EXPORT void home(context context);
BOOST_SYMBOL_EXPORT void hello(context context, const std::string& name);

}
}

#endif // UDHO_EXAMPLES_HELLO_ACTIONS_H
