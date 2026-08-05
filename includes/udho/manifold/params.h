#ifndef UDHO_MANIFOLD_CONFIG_PARAMS_H
#define UDHO_MANIFOLD_CONFIG_PARAMS_H

#include <udho/manifold/fwd.h>
#include <udho/hazo/map.h>
#include <udho/hazo/string/basic.h>
#include <nlohmann/json.hpp>
#include <udho/utils/traits.h>

/**
 * @addtogroup DoxyG_manifold
 * @{
 */

/**
 * @def UDHO_CONFIG_PARAM(Name, Type, DefaultValue)
 * @brief Macro for declaring configuration parameters
 *
 * Generates a configuration parameter type with:
 * - Default value initialization
 * - Type-safe value storage
 *
 * # Usage
 *
 * @code
 * // Declare parameters for a component
 * struct MyComponent {
 *     UDHO_CONFIG_PARAM(timeout,  int,         5000);
 *     UDHO_CONFIG_PARAM(enabled,  bool,        true);
 *     UDHO_CONFIG_PARAM(endpoint, std::string, "http://localhost");
 *
 *     using params = udho::manifold::params<timeout, enabled, endpoint>;
 * };
 * @endcode
 *
 * # Generated Structure
 *
 * For `UDHO_CONFIG_PARAM(timeout, int, 5000)`, generates:
 * @code
 * struct timeout : udho::hazo::element<timeout, int> {
 *     timeout() : base_element(5000) {}
 *     explicit timeout(const int& value) : base_element(value) {}
 *     static constexpr auto key() { return "timeout"_h; }
 * };
 * @endcode
 *
 * @param Name Parameter identifier (becomes struct name)
 * @param Type C++ type for the parameter value
 * @param DefaultValue Default value (used if not specified in config)
 *
 * @see params
 * @see config
 */
#define UDHO_CONFIG_PARAM(Name, Type, DefaultValue)                     \
struct Name: udho::hazo::element<Name , Type>{                          \
        using base_element = udho::hazo::element<Name , Type>;          \
        using base_element::base_element;                               \
        inline Name(): base_element(DefaultValue) {}                    \
        using base_element::operator=;                                  \
        inline static constexpr auto key() {                            \
            using namespace udho::hazo::string::literals;               \
            return #Name##_h;                                           \
    }                                                                   \
}

/**
 * @}
 */

namespace udho{
namespace manifold{
namespace detail {


struct json_serializer{
    inline json_serializer(nlohmann::json& json): _json(json) {}

    template <typename ParamT>
    void operator()(const ParamT& d){
        const auto& v = d.value();
        if constexpr (udho::utils::traits::is_optional<typename ParamT::value_type>::value) {
            if(v.has_value()) {
                _json[d.key().c_str()] = v;
            }
        } else {
            _json[d.key().c_str()] = v;
        }
    }

    nlohmann::json& _json;

private:
    template <typename ValueT>
    void serialize(const char* key, std::optional<ValueT>) {

    }
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

/**
 * @addtogroup DoxyG_manifold
 * @{
 */

/**
 * @brief Type-safe parameter storage with JSON serialization
 *
 * Provides a compile-time map from parameter types to their values. Parameters
 * are accessed by type, ensuring type safety and preventing typos.
 *
 * @tparam Fields... Parameter types (defined via UDHO_CONFIG_PARAM)
 *
 * # Parameter Definition
 *
 * @code
 * struct MyComponent {
 *     UDHO_CONFIG_PARAM(max_retries, int,         3);
 *     UDHO_CONFIG_PARAM(timeout_ms,  int,         1000);
 *     UDHO_CONFIG_PARAM(debug_mode,  bool,        false);
 *     UDHO_CONFIG_PARAM(server_url,  std::string, "localhost");
 *
 *     using params = udho::manifold::params<
 *         max_retries,
 *         timeout_ms,
 *         debug_mode,
 *         server_url
 *     >;
 * };
 * @endcode
 *
 * # JSON Persistence
 *
 * Parameters can be saved to and loaded from JSON:
 * @code
 * MyComponent::params p;
 *
 * // Serialize to JSON
 * nlohmann::json json;
 * p.save(json);
 * // Result: {
 * //   "max_retries": 3,
 * //   "timeout_ms": 1000,
 * //   "debug_mode": false,
 * //   "server_url": "localhost"
 * // }
 *
 * // Deserialize from JSON
 * nlohmann::json config = {
 *     {"max_retries", 5},
 *     {"debug_mode", true}
 * };
 * p.load(config);
 * // max_retries=5, debug_mode=true, others unchanged
 * @endcode
 *
 * # Parameter Access
 *
 * Access is type-based via the hazo::map_d interface:
 * @code
 * params p;
 * p[max_retries::val] = 10;
 * int retries = p[max_retries::val].value();
 * @endcode
 *
 * @note Empty params<> specialization is provided for components without config
 *
 * @see config
 * @see UDHO_CONFIG_PARAM
 */
template <typename... Fields>
struct params: udho::hazo::map_d<Fields...>{
    using map_type = udho::hazo::map_d<Fields...>;

    using map_type::map_type;

    /**
     * @brief Serialize parameters to JSON
     *
     * Creates a JSON object with one property per parameter, using each
     * parameter's key() as the property name.
     *
     * @param json Output JSON object (will be initialized as object)
     *
     * @post json.is_object() == true
     * @post json contains all parameters with their current values
     */
    void save(nlohmann::json& json) const {
        json = nlohmann::json::object();
        map_type::visit(detail::json_serializer{json});
    }

    /**
     * @brief Deserialize parameters from JSON
     *
     * Loads parameter values from JSON object. Missing properties are ignored
     * (parameters retain their current values). Extra properties are ignored.
     *
     * @param json Input JSON object containing parameter values
     *
     * @pre json.is_object() == true
     *
     * # Partial Updates
     *
     * @code
     * params p;  // All default values
     *
     * nlohmann::json partial = {{"max_retries", 10}};
     * p.load(partial);  // Only max_retries updated
     * @endcode
     */
    void load(const nlohmann::json& json) {
        assert(json.is_object());
        map_type::visit(detail::json_deserializer{json});
    }

    template <typename Function>
    void visit(Function&& f) {
        map_type::visit(std::forward<Function>(f));
    }

    template <typename Function>
    void visit(Function&& f) const {
        map_type::visit(std::forward<Function>(f));
    }
};

#ifndef __DOXYGEN__

template <>
struct params<>{
    void save(nlohmann::json& json) const {
        json = nlohmann::json::object();
    }
    void load(const nlohmann::json& json) {
        assert(json.is_object());
    }

    template <typename Function>
    void visit(Function&& f) const {}

    template <typename>
    struct contains {
        enum {
            value = false
        };
    };
};

#endif // __DOXYGEN__

namespace detail {

template <typename FieldT>
struct field: udho::hazo::element<field<FieldT>, bool>{
    using element_type = typename udho::hazo::element<field<FieldT>, bool>;

    using element_type::element;
    using element_type::operator=;
    static constexpr auto key() { return element_type::val; }
};

}

/**
 * @}
 */


}
}


#endif // UDHO_MANIFOLD_CONFIG_PARAMS_H
