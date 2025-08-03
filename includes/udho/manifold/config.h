#ifndef UDHO_MANIFOLD_CONFIG_H
#define UDHO_MANIFOLD_CONFIG_H

#include <nlohmann/json_fwd.hpp>
#include <udho/manifold/fwd.h>
#include <udho/hazo/map.h>
#include <udho/hazo/string/basic.h>
#include <nlohmann/json.hpp>
#include <udho/manifold/traits.h>

#define UDHO_CONFIG_PARAM(Name, Type, DefaultValue)             \
struct Name: udho::hazo::element<Name , Type>{                  \
    inline Name(): element(DefaultValue) {}                     \
    inline explicit Name(const Type& value): element(value) {}  \
    Name(const Name&) = default;                                \
    Name(Name&&) = default;                                     \
    using element::operator=;                                   \
    inline static constexpr auto key() {                        \
        using namespace udho::hazo::string::literals;           \
        return #Name##_h;                                       \
    }                                                           \
}

namespace udho{
namespace manifold{

namespace detail {

struct json_serializer{
    inline json_serializer(nlohmann::json& json): _json(json) {}

    template <typename ParamT>
    void operator()(const ParamT& d){ _json[d.key().c_str()] = d.value(); }

    nlohmann::json& _json;
};

struct json_deserializer{
    inline json_deserializer(const nlohmann::json& json): _json(json) {}

    template <typename ParamT>
    void operator()(ParamT& d){ d = _json[d.key().c_str()]; }

    const nlohmann::json& _json;
};

}


template <typename... Fields>
struct params: udho::hazo::map_d<Fields...>{
    typedef udho::hazo::map_d<Fields...> map_type;
    using map_type::map_type;

    void save(nlohmann::json& json) {
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
    void save(nlohmann::json& json) {}
    void load(const nlohmann::json& json) {}
};

template <typename ComponentT>
struct basic_config{
    using components_type = ComponentT;
    using params_type = typename udho::manifold::component_traits<ComponentT>::params;

    void save(nlohmann::json& json) {
        auto params = nlohmann::json::object();
        _params.save(params);
        assert(json.is_object());
        json[ComponentT::name]= params;
    }

    void load(const nlohmann::json& json) {
        assert(json.is_object());
        assert(json.contains(ComponentT::name));
        _params.load(json[ComponentT::name]);
    }

private:
    params_type _params;
};


/**
 * @brief The config specialization provides facilities to con mantain configuration of a component
 */
template <typename ComponentT>
struct config: basic_config<ComponentT>{
    bool valid() const { return true; }
};


}
}

#endif // UDHO_MANIFOLD_CONFIG_H
