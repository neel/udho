#ifndef UDHO_WWW_PROFILE_H
#define UDHO_WWW_PROFILE_H

#include <udho/net/channel.h>
#include <udho/net/transport.h>
#include <udho/www/presets.h>

namespace udho {
namespace www {

/**
 * @brief Binds a network transport to stream-specific WWW presets.
 *
 * The profile exposes complete labels through `stateless` and `stateful`, and
 * exposes the configured, user-instantiable channel as `transport`.
 *
 * @tparam TransportT Network transport metadata.
 */
template <typename TransportT>
struct basic_profile {
    using transport_type = TransportT;
    using transport      = udho::net::channel<transport_type>;
    using preset_type    = presets<typename transport_type::stream_type>;
    using stateless      = typename preset_type::stateless;
    using stateful       = typename preset_type::stateful;
};

/** @brief WWW labels and configured transport for plain HTTP. */
using http = basic_profile<udho::net::transports::http>;

/** @brief WWW labels and configured transport for HTTPS. */
using https = basic_profile<udho::net::transports::https>;

/** @brief WWW labels and configured transport for local stream sockets. */
using local = basic_profile<udho::net::transports::local>;

/** @brief WWW labels and configured transport for Beast test streams. */
using testing = basic_profile<udho::net::transports::testing>;

}
}

#endif // UDHO_WWW_PROFILE_H
