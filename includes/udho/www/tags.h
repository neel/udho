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

template <typename... Bridges>
struct minimal{
    using resources_component_type = udho::www::components::resources<Bridges...>;
};

template <typename SessionStorageT, udho::session::modes Mode, typename... Bridges>
struct statefulx {
    using sesssion_storage_type = SessionStorageT;
    static constexpr const udho::session::modes session_storage_mode = Mode;
    using resources_component_type = udho::www::components::resources<Bridges...>;
};

template <typename SessionStorageT = udho::session::storage::fs, udho::session::modes Mode = udho::session::modes::lazy>
using stateful = statefulx<SessionStorageT, Mode>;

}

}
}

#endif // UDHO_WWW_TAGS_H
