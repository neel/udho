// SPDX-License-Identifier: BSD-3-Clause

#include "data.h"

luax::data::page_data luax::data::home() {
    return {
        "Lua views with udho",
        "visitor",
        "This content came from a C++ object rendered by a Lua view."
    };
}

luax::data::page_data luax::data::hello(const std::string& name) {
    return {
        "A personalized Lua view",
        name,
        "The standard layout rendered this view in its central placeholder."
    };
}

