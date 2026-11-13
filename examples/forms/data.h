// SPDX-License-Identifier: BSD-3-Clause

#ifndef UDHO_EXAMPLES_FORMS_DATA_H
#define UDHO_EXAMPLES_FORMS_DATA_H

#include <string>
#include <vector>
#include <udho/view/data.h>
#include <udho/view/meta.h>

namespace forms::data {

struct field {
    std::string name;
    std::string value;

    friend auto metatype(udho::view::data::type<field>) {
        using namespace udho::view::data;
        return assoc("form_field"), cvar("name", &field::name), cvar("value", &field::value);
    }
};

struct page {
    std::string title;
    std::string message;
    std::vector<field> fields;

    friend auto metatype(udho::view::data::type<page>) {
        using namespace udho::view::data;
        return assoc("form_page"),
               cvar("title", &page::title),
               cvar("message", &page::message),
               cvar("fields", &page::fields);
    }
};

}

#endif // UDHO_EXAMPLES_FORMS_DATA_H
