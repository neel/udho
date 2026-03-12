#ifndef UDHO_WWW_PRESETS_H
#define UDHO_WWW_PRESETS_H

#include <udho/www/tags.h>
#include <udho/www/label.h>
#include <udho/session/defs.h>
#include <udho/session/storage/fs.h>
#include <udho/session/storage/redis.h>
#include <udho/session/storage/fs_mem.h>
#include <udho/view/bridges/lua.h>

namespace udho {
namespace www {

// { convenience labels

namespace stateless {
    using rest                  = www::label<www::tags::minimal<>>;
    using lua                   = www::label<www::tags::minimal<udho::view::data::bridges::lua>>;
}

namespace stateful {
    using lazy_fs               = www::label<www::tags::stateful<udho::session::storage::fs, udho::session::modes::lazy>>;
    using optimistic_fs         = www::label<www::tags::stateful<udho::session::storage::fs, udho::session::modes::optimistic>>;
    using lazy_memfs            = www::label<www::tags::stateful<udho::session::storage::fs, udho::session::modes::lazy>>;
    using optimistic_memfs      = www::label<www::tags::stateful<udho::session::storage::fs, udho::session::modes::optimistic>>;
    using lazy_redis            = www::label<www::tags::stateful<udho::session::storage::redis, udho::session::modes::lazy>>;
    using optimistic_redis      = www::label<www::tags::stateful<udho::session::storage::redis, udho::session::modes::optimistic>>;
    using immediate_redis       = www::label<www::tags::stateful<udho::session::storage::redis, udho::session::modes::immediate>>;

    namespace lua {
        using lazy_fs           = www::label<www::tags::statefulx<udho::session::storage::fs, udho::session::modes::lazy, udho::view::data::bridges::lua>>;
        using optimistic_fs     = www::label<www::tags::statefulx<udho::session::storage::fs, udho::session::modes::optimistic, udho::view::data::bridges::lua>>;
        using lazy_memfs        = www::label<www::tags::statefulx<udho::session::storage::fs, udho::session::modes::lazy, udho::view::data::bridges::lua>>;
        using optimistic_memfs  = www::label<www::tags::statefulx<udho::session::storage::fs, udho::session::modes::optimistic, udho::view::data::bridges::lua>>;
        using lazy_redis        = www::label<www::tags::statefulx<udho::session::storage::redis, udho::session::modes::lazy, udho::view::data::bridges::lua>>;
        using optimistic_redis  = www::label<www::tags::statefulx<udho::session::storage::redis, udho::session::modes::optimistic, udho::view::data::bridges::lua>>;
        using immediate_redis   = www::label<www::tags::statefulx<udho::session::storage::redis, udho::session::modes::immediate, udho::view::data::bridges::lua>>;
    }
}

// }


}
}

#endif // UDHO_WWW_PRESETS_H
