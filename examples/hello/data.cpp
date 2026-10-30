// SPDX-License-Identifier: BSD-3-Clause

#include "data.h"

hello::data::page_data hello::data::home() {
    return {
        "Lua views with udho",
        "visitor",
        "This content came from a C++ object rendered by a Lua view.",
        {}
    };
}

hello::data::page_data hello::data::hello(const std::string& name) {
    return {
        "A personalized Lua view",
        name,
        "The standard layout rendered this view in its central placeholder.",
        {
            planet{"Mercury", 91691000,   "#777674"},
            planet{"Venus",   41400000,   "#b97832"},
            planet{"Earth",   0,          "#2878b8"},
            planet{"Mars",    78340000,   "#c65d3b"},
            planet{"Jupiter", 628730000,  "#c28a42"},
            planet{"Saturn",  1275000000, "#b89552"},
            planet{"Uranus",  2723950000, "#4c9ba8"},
            planet{"Neptune", 4351400000, "#3f5fae"}
        }
    };
}

