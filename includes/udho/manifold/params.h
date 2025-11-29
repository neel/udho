#ifndef UDHO_MANIFOLD_CONFIG_PARAMS_H
#define UDHO_MANIFOLD_CONFIG_PARAMS_H

#include <udho/manifold/fwd.h>
#include <udho/hazo/map.h>
#include <udho/hazo/string/basic.h>
#include <nlohmann/json.hpp>

#define UDHO_CONFIG_PARAM(Name, Type, DefaultValue)                     \
struct Name: udho::hazo::element<Name , Type>{                          \
        using base_element = udho::hazo::element<Name , Type>;          \
        inline Name(): base_element(DefaultValue) {}                    \
        inline explicit Name(const Type& value): base_element(value) {} \
        Name(const Name&) = default;                                    \
        Name(Name&&) = default;                                         \
        using base_element::operator=;                                  \
        inline static constexpr auto key() {                            \
            using namespace udho::hazo::string::literals;               \
            return #Name##_h;                                           \
    }                                                                   \
}

namespace udho{
namespace manifold{
namespace detail {

struct json_serializer{
    inline json_serializer(nlohmann::json& json): _json(json) {}

    template <typename ParamT>
    void operator()(const ParamT& d){
        _json[d.key().c_str()] = d.value();
    }

    nlohmann::json& _json;
};

struct json_deserializer{
    inline json_deserializer(const nlohmann::json& json): _json(json) {}

    template <typename ParamT>
    bool operator()(ParamT& d){
        const auto& key = d.key();
        if(_json.contains(key.c_str())){
            d = _json[key.c_str()];
            return true;
        }
        return false;
    }

    const nlohmann::json& _json;
};

}


template <typename... Fields>
struct params: udho::hazo::map_d<Fields...>{
    using map_type = udho::hazo::map_d<Fields...>;

    using map_type::map_type;

    void save(nlohmann::json& json) const {
        json = nlohmann::json::object();
        map_type::visit(detail::json_serializer{json});
    }

    void load(const nlohmann::json& json) {
        assert(json.is_object());
        map_type::visit(detail::json_deserializer{json});
    }
};

template <>
struct params<>{
    void save(nlohmann::json& json) const {
        json = nlohmann::json::object();
    }
    void load(const nlohmann::json& json) {
        assert(json.is_object());
    }

    template <typename>
    using contains = std::false_type;
};

namespace detail {

template <typename FieldT>
struct field: udho::hazo::element<field<FieldT>, bool>{
    using element_type = typename udho::hazo::element<field<FieldT>, bool>;

    using element_type::element;
    using element_type::operator=;
    static constexpr auto key() { return element_type::val; }
};

}

}
}

#endif // UDHO_MANIFOLD_CONFIG_PARAMS_H
