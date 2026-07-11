#ifndef UDHO_WWW_TAGS_H
#define UDHO_WWW_TAGS_H

#include <udho/www/components/resources.h>
#include <udho/session/defs.h>
#include <udho/session/storage/fs.h>
#include <udho/session/storage/redis.h>
#include <udho/session/storage/fs_mem.h>

namespace udho {
namespace www {

namespace tags{

/**
 * @brief Tag selecting the minimal/stateless www sketch.
 *
 * The tag stores the resources component type parameterized by the supplied
 * view-data bridge types. It is consumed by the `sketch` specialization for
 * minimal www applications.
 *
 * @tparam Bridges View-data bridge types exposed through the resources component.
 *
 * @ingroup www
 */
template <typename... Bridges>
struct minimal{
    using resources_component_type = udho::www::components::resources<Bridges...>;
};

/**
 * @brief Tag selecting the stateful www sketch.
 *
 * The tag records the session storage backend, session mode, and resources
 * bridge configuration used by the stateful www sketch.
 *
 * @tparam SessionStorageT Session storage backend type.
 * @tparam Mode Session persistence mode.
 * @tparam Bridges View-data bridge types exposed through the resources component.
 *
 * @ingroup www
 */
template <typename SessionStorageT, udho::session::modes Mode, typename... Bridges>
struct statefulx {
    using sesssion_storage_type = SessionStorageT;
    static constexpr const udho::session::modes session_storage_mode = Mode;
    using resources_component_type = udho::www::components::resources<Bridges...>;
};

/**
 * @brief Convenience stateful tag alias.
 *
 * Defaults to filesystem session storage in lazy mode.
 *
 * @tparam SessionStorageT Session storage backend type.
 * @tparam Mode Session persistence mode.
 *
 * @ingroup www
 */
template <typename SessionStorageT = udho::session::storage::fs, udho::session::modes Mode = udho::session::modes::lazy>
using stateful = statefulx<SessionStorageT, Mode>;

}

}
}

#endif // UDHO_WWW_TAGS_H
