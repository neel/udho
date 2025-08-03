#ifndef UDHO_MANIFOLD_COMPONENTS_X_H
#define UDHO_MANIFOLD_COMPONENTS_X_H

#include <udho/manifold/config.h>

namespace udho{
namespace manifold{

namespace components{

/**
 * @brief A ComponentX instance serves as the persistent entity having a full application lifetime
 * @details responsible for maintining the component and provide definitions on how this component may be used
 *          such as configuration parameters, state typedef etc...
 */
struct ComponentX{
    UDHO_CONFIG_PARAM(hostname, std::string,    "localhost" );
    UDHO_CONFIG_PARAM(port,     std::uint32_t,  3306        );
    UDHO_CONFIG_PARAM(username, std::string,    "root"      );
    UDHO_CONFIG_PARAM(password, std::string,    ""          );

    static constexpr const char* name = "x";

    using params = udho::manifold::params<hostname, port, username, password>;
};

}
}
}

namespace udho{
namespace manifold{

/**
 * @brief The delegate class is created per connection
 * @details it is created with a mutable reference to the persistent component instance and immutable configuration
 *          responsible for providing usercode facilities to interact with teh component while adhering to the specific
 *          configurations
 */
template <>
struct delegate<components::ComponentX>{
    using config_type = config<components::ComponentX>;

    delegate(components::ComponentX& component, const config_type& config): _component(component), _config(config) {}

private:
    components::ComponentX& _component;
    const config_type& _config;
};

}
}

#endif // UDHO_MANIFOLD_COMPONENTS_X_H
