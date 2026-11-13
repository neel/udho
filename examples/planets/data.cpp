// SPDX-License-Identifier: BSD-3-Clause

#include "data.h"

planets::data::page planets::data::home() {
    page data;
    data.heading("Lua views with udho")
        .name("visitor")
        .message("This content came from a C++ object rendered by a Lua view.");
    return data;
}

planets::data::page planets::data::hello(const std::string& name) {
    page data;
    data.heading("A personalized Lua view")
        .name(name)
        .message("The standard layout rendered this view in its central placeholder.");
    data << planet("Mercury").distance(91691000).color("#777674")
         << planet("Venus").distance(41400000).color("#b97832")
         << planet("Earth").distance(0).color("#2878b8")
         << planet("Mars").distance(78340000).color("#c65d3b")
         << planet("Jupiter").distance(628730000).color("#c28a42")
         << planet("Saturn").distance(1275000000).color("#b89552")
         << planet("Uranus").distance(2723950000).color("#4c9ba8")
         << planet("Neptune").distance(4351400000).color("#3f5fae");
    return data;
}

