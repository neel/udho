// SPDX-License-Identifier: BSD-3-Clause

#ifndef UDHO_EXAMPLES_LUAX_DATA_H
#define UDHO_EXAMPLES_LUAX_DATA_H

#include <string>
#include <udho/view/data.h>
#include <udho/view/meta.h>

namespace luax {
namespace data {

struct page_data {
    std::string heading;
    std::string name;
    std::string message;

    friend auto metatype(udho::view::data::type<page_data>) {
        using namespace udho::view::data;

        return assoc("luax_page_data"),
               cvar("heading",  &page_data::heading),
               cvar("name",     &page_data::name),
               cvar("message",  &page_data::message);
    }
};

page_data home();
page_data hello(const std::string& name);

}
}

#endif // UDHO_EXAMPLES_LUAX_DATA_H

