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
    planet() = default;
    explicit planet(std::string name): _name(std::move(name)) {}
    planet(std::string name, std::uint64_t distance, std::string color): _name(std::move(name)), _distance(distance), _color(std::move(color)) {}

    const std::string& name() const { return _name; }
    std::uint64_t distance() const { return _distance; }
    const std::string& color() const { return _color; }

    planet& name(std::string name) { _name = std::move(name); return *this; }
    planet& distance(std::uint64_t distance) { _distance = distance; return *this; }
    planet& color(std::string color) { _color = std::move(color); return *this; }

    friend auto metatype(udho::view::data::type<planet>) {
        using namespace udho::view::data;

        return assoc("planet"),
               cvar("name",     &planet::_name),
               cvar("distance", &planet::_distance),
               cvar("color",    &planet::_color);
    }

private:
    std::string _name;
    std::uint64_t _distance = 0;
    std::string _color;
};

class page {
public:
    const std::string& heading() const { return _heading; }
    const std::string& name() const { return _name; }
    const std::string& message() const { return _message; }
    const std::vector<planet>& planets() const { return _planets; }

    page& heading(std::string heading) { _heading = std::move(heading); return *this; }
    page& name(std::string name) { _name = std::move(name); return *this; }
    page& message(std::string message) { _message = std::move(message); return *this; }
    page& add(planet value) { _planets.push_back(std::move(value)); return *this; }
    page& operator<<(planet value) { return add(std::move(value)); }

    friend auto metatype(udho::view::data::type<page>) {
        using namespace udho::view::data;

        return assoc("page"),
               cvar("heading",  &page::_heading),
               cvar("name",     &page::_name),
               cvar("message",  &page::_message),
               cvar("planets",  &page::_planets);
    }

private:
    std::string _heading;
    std::string _name;
    std::string _message;
    std::vector<planet> _planets;
};

page home();
page hello(const std::string& name);

}
}

#endif // UDHO_EXAMPLES_HELLO_DATA_H

