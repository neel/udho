// SPDX-License-Identifier: BSD-3-Clause

#ifndef UDHO_EXAMPLES_HELLO_DATA_H
#define UDHO_EXAMPLES_HELLO_DATA_H

#include <cstdint>
#include <string>
#include <utility>
#include <vector>
#include <udho/view/data.h>
#include <udho/view/meta.h>

namespace hello {
namespace data {

class planet {
public:
    planet(std::string name, std::uint64_t distance, std::string color)
        : _name(std::move(name)), _distance(distance), _color(std::move(color)) {}

    const std::string& name() const { return _name; }
    std::uint64_t distance() const { return _distance; }
    const std::string& color() const { return _color; }

    friend auto metatype(udho::view::data::type<planet>) {
        using namespace udho::view::data;

        return assoc("planet"),
               fvar("name",     &planet::name),
               fvar("distance", &planet::distance),
               fvar("color",    &planet::color);
    }

private:
    std::string _name;
    std::uint64_t _distance;
    std::string _color;
};

struct page_data {
    std::string heading;
    std::string name;
    std::string message;
    std::vector<planet> planets;

    friend auto metatype(udho::view::data::type<page_data>) {
        using namespace udho::view::data;

        return assoc("page_data"),
               cvar("heading",  &page_data::heading),
               cvar("name",     &page_data::name),
               cvar("message",  &page_data::message),
               cvar("planets",  &page_data::planets);
    }
};

page_data home();
page_data hello(const std::string& name);

}
}

#endif // UDHO_EXAMPLES_HELLO_DATA_H

