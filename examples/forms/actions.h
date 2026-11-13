// SPDX-License-Identifier: BSD-3-Clause

#ifndef UDHO_EXAMPLES_FORMS_ACTIONS_H
#define UDHO_EXAMPLES_FORMS_ACTIONS_H

#include <udho/net/common.h>
#include <udho/view/bridges/lua.h>
#include <udho/www/components/protocol.h>
#include <udho/www/components/resources.h>
#include <udho/www/context.h>

namespace forms {
namespace actions {

using protocol_type = udho::www::components::protocols::http<udho::net::types::socket>;
using lua_bridge = udho::view::data::bridges::lua;
using context = udho::www::context<protocol_type, udho::www::components::resources<lua_bridge>>;

void home(context context);
void submit(context context);
void json(context context);
void upload(context context);

}
}

#endif // UDHO_EXAMPLES_FORMS_ACTIONS_H
