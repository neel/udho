// SPDX-License-Identifier: BSD-3-Clause

#ifndef UDHO_EXAMPLES_FORMS_ACTIONS_H
#define UDHO_EXAMPLES_FORMS_ACTIONS_H

#include <udho/view/bridges/lua.h>
#include <udho/www/components/request.h>
#include <udho/www/components/resources.h>
#include <udho/www/context.h>

namespace forms {
namespace actions {

using lua_bridge = udho::view::data::bridges::lua;
using context = udho::www::context<udho::www::components::request, udho::www::components::resources<lua_bridge>>;

void home(context context);
void submit(context context);
void json(context context);
void upload(context context);

}
}

#endif // UDHO_EXAMPLES_FORMS_ACTIONS_H
