#ifndef UDHO_MANIFOLD_CONFIG_H
#define UDHO_MANIFOLD_CONFIG_H

#include <nlohmann/json_fwd.hpp>
#include <udho/manifold/fwd.h>
#include <udho/hazo/map.h>
#include <udho/hazo/string/basic.h>
#include <nlohmann/json.hpp>
#include <udho/manifold/traits.h>

#define UDHO_CONFIG_PARAM(Name, Type, DefaultValue)                 \
struct Name: udho::hazo::element<Name , Type>{                      \
    using base_element = udho::hazo::element<Name , Type>;          \
    inline Name(): base_element(DefaultValue) {}                    \
    inline explicit Name(const Type& value): base_element(value) {} \
    Name(const Name&) = default;                                    \
    Name(Name&&) = default;                                         \
    using base_element::operator=;                                  \
    inline static constexpr auto key() {                            \
        using namespace udho::hazo::string::literals;               \
        return #Name##_h;                                           \
    }                                                               \
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

template <typename ComponentT>
struct basic_config{
    using components_type = ComponentT;
    using params_type = typename udho::manifold::component_traits<ComponentT>::params;

    template <typename ParamT>
    using contains = typename params_type::template contains<ParamT>;


    /**
     * @brief save
     * @param json
     * @pre expects the provided json is an object
     */
    void save(nlohmann::json& json) const {
        auto params = nlohmann::json::object();
        _params.save(params);
        assert(json.is_object());
        json[udho::manifold::component_name<ComponentT>()]= params;
    }

    void load(const nlohmann::json& json) {
        assert(json.is_object());
        assert(json.contains(ComponentT::name));
        _params.load(json[udho::manifold::component_name<ComponentT>()]);
    }

    template <typename ParamT>
    auto& operator[](const udho::hazo::element_t<ParamT>& key){ return _params[key]; }

    template <typename ParamT>
    const auto& operator[](const udho::hazo::element_t<ParamT>& key) const { return _params[key]; }

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

template <typename... Params>
struct changeset: udho::hazo::map_d<Params...>{
    typedef udho::hazo::map_d<Params...> map_type;

    using map_type::map_type;

    template <typename ComponentT>
    void mutate(config<ComponentT>& config) {
        map_type::visit(mutate_{config});
    }

private:
    template <typename ComponentT>
    struct mutate_{
        using config_type = config<ComponentT>;

        mutate_(config_type& config): _config(config) {}

        template <typename ParamT, std::enable_if_t<config_type::template contains<ParamT>::value, bool> = true>
        void operator()(const ParamT& param) {
            _config[udho::hazo::element_t<ParamT>{}] = param.value();
        }

        template <typename ParamT, std::enable_if_t<!config_type::template contains<ParamT>::value, bool> = true>
        void operator()(const ParamT& param) { }

        config_type& _config;
    };
};

template <typename...>
class configs;


template <typename... Params>
changeset<Params...> make_changeset(Params&&... params){
    return changeset<Params...>{params...};
}

template <typename ComponentT, typename... Components>
class configs<ComponentT, Components...>: configs<Components...> {
    using config_type = config<ComponentT>;
    using rest_type   = configs<Components...>;

    config_type _config;

public:
    template <typename ParamT, std::enable_if_t<config_type::template contains<ParamT>::value, bool> = true>
    auto& operator[](const udho::hazo::element_t<ParamT>& key){ return _config[key]; }

    template <typename ParamT, std::enable_if_t<config_type::template contains<ParamT>::value, bool> = true>
    const auto& operator[](const udho::hazo::element_t<ParamT>& key) const { return _config[key]; }

    template <typename ParamT, std::enable_if_t<!config_type::template contains<ParamT>::value, bool> = true>
    auto& operator[](const udho::hazo::element_t<ParamT>& key){ return rest_type::template operator[]<ParamT>(key); }

    template <typename ParamT, std::enable_if_t<!config_type::template contains<ParamT>::value, bool> = true>
    const auto& operator[](const udho::hazo::element_t<ParamT>& key) const { return rest_type::template operator[]<ParamT>(key); }

    template <typename... Params>
    void patch(const changeset<Params...>& changeset) {

    }
};



template <typename ParamsT, typename ChangesetT>
struct patch;

template <typename... Fields, typename... Params>
struct patch<udho::manifold::params<Fields...>, udho::manifold::changeset<Params...>> {
    using params_type    = udho::manifold::params<Fields...>;
    using changeset_type = udho::manifold::changeset<Params...>;

    patch(const params_type& params, const changeset_type& changeset): _params(params), _changeset(changeset) {}

    template <typename ParamT, std::enable_if_t<changeset_type::template contains<ParamT>::value, bool> = true>
    auto& operator[](const udho::hazo::element_t<ParamT>& key){
        return _changeset[key];
    }

    template <typename ParamT, std::enable_if_t<changeset_type::template contains<ParamT>::value, bool> = true>
    const auto& operator[](const udho::hazo::element_t<ParamT>& key) const {
        return _changeset[key];
    }

    template <typename ParamT, std::enable_if_t<!changeset_type::template contains<ParamT>::value, bool> = true>
    auto& operator[](const udho::hazo::element_t<ParamT>& key){
        return _params[key];
    }

    template <typename ParamT, std::enable_if_t<!changeset_type::template contains<ParamT>::value, bool> = true>
    const auto& operator[](const udho::hazo::element_t<ParamT>& key) const {
        return _params[key];
    }

    private:
    const params_type& _params;
    const changeset_type& _changeset;
};

template <typename... Fields, typename... Params>
patch<udho::manifold::params<Fields...>, udho::manifold::changeset<Params...>> make_patch(const udho::manifold::params<Fields...>& params, const udho::manifold::changeset<Params...>& changeset){
    return patch<udho::manifold::params<Fields...>, udho::manifold::changeset<Params...>>{params, changeset};
}


namespace detail {

template <typename FieldT>
struct field: udho::hazo::element<field<FieldT>, bool>{
    using element_type = typename udho::hazo::element<field<FieldT>, bool>;

    using element_type::element;
    using element_type::operator=;
    static constexpr auto key() { return element_type::val; }
};

}

template <typename... Fields, typename... Params>
udho::hazo::map_d<detail::field<Fields>...> make_diff(const udho::manifold::params<Fields...>& params, const udho::manifold::changeset<Params...>& changeset){
    using changeset_type = udho::manifold::changeset<Params...>;
    return udho::hazo::map_d<detail::field<Fields>...>{changeset_type::template contains<Fields>::value...};
}

}
}

#endif // UDHO_MANIFOLD_CONFIG_H
