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


// { convenience

template <typename StreamT>
struct presets{
    /** @brief Convenience labels for stateless www applications. */
    struct stateless {
        /**
         * @brief Stateless REST-style www label with no view bridge.
         */
        using rest                  = www::basic_label<StreamT, www::tags::minimal<>>;
        /**
         * @brief Stateless www label with Lua view-data bridge support.
         */
        using lua                   = www::basic_label<StreamT, www::tags::minimal<udho::view::data::bridges::lua>>;
    };

    /** @brief Convenience labels for session-enabled www applications. */
    struct stateful {
        /**
         * @brief Stateful label using filesystem session storage in lazy mode.
         */
        using lazy_fs               = www::basic_label<StreamT, www::tags::stateful<udho::session::storage::fs, udho::session::modes::lazy>>;
        /**
         * @brief Stateful label using filesystem session storage in optimistic mode.
         */
        using optimistic_fs         = www::basic_label<StreamT, www::tags::stateful<udho::session::storage::fs, udho::session::modes::optimistic>>;
        /**
         * @brief Stateful label using memory-backed filesystem session storage in lazy mode.
         */
        using lazy_memfs            = www::basic_label<StreamT, www::tags::stateful<udho::session::storage::fs, udho::session::modes::lazy>>;
        /**
         * @brief Stateful label using memory-backed filesystem session storage in optimistic mode.
         */
        using optimistic_memfs      = www::basic_label<StreamT, www::tags::stateful<udho::session::storage::fs, udho::session::modes::optimistic>>;

#ifdef WITH_HIREDIS
        /**
         * @brief Stateful label using Redis session storage in lazy mode.
         */
        using lazy_redis            = www::basic_label<StreamT, www::tags::stateful<udho::session::storage::redis, udho::session::modes::lazy>>;
        /**
         * @brief Stateful label using Redis session storage in optimistic mode.
         */
        using optimistic_redis      = www::basic_label<StreamT, www::tags::stateful<udho::session::storage::redis, udho::session::modes::optimistic>>;
        /**
         * @brief Stateful label using Redis session storage in immediate mode.
         */
        using immediate_redis       = www::basic_label<StreamT, www::tags::stateful<udho::session::storage::redis, udho::session::modes::immediate>>;

#endif // WITH_HIREDIS
        /** @brief Convenience labels for session-enabled www applications with Lua view support. */
        struct lua {
            /**
             * @brief Stateful Lua label using filesystem session storage in lazy mode.
             */
            using lazy_fs           = www::basic_label<StreamT, www::tags::statefulx<udho::session::storage::fs, udho::session::modes::lazy, udho::view::data::bridges::lua>>;
            /**
             * @brief Stateful Lua label using filesystem session storage in optimistic mode.
             */
            using optimistic_fs     = www::basic_label<StreamT, www::tags::statefulx<udho::session::storage::fs, udho::session::modes::optimistic, udho::view::data::bridges::lua>>;
            /**
             * @brief Stateful Lua label using memory-backed filesystem session storage in lazy mode.
             */
            using lazy_memfs        = www::basic_label<StreamT, www::tags::statefulx<udho::session::storage::fs, udho::session::modes::lazy, udho::view::data::bridges::lua>>;
            /**
             * @brief Stateful Lua label using memory-backed filesystem session storage in optimistic mode.
             */
            using optimistic_memfs  = www::basic_label<StreamT, www::tags::statefulx<udho::session::storage::fs, udho::session::modes::optimistic, udho::view::data::bridges::lua>>;

#ifdef WITH_HIREDIS
            /**
             * @brief Stateful Lua label using Redis session storage in lazy mode.
             */
            using lazy_redis        = www::basic_label<StreamT, www::tags::statefulx<udho::session::storage::redis, udho::session::modes::lazy, udho::view::data::bridges::lua>>;
            /**
             * @brief Stateful Lua label using Redis session storage in optimistic mode.
             */
            using optimistic_redis  = www::basic_label<StreamT, www::tags::statefulx<udho::session::storage::redis, udho::session::modes::optimistic, udho::view::data::bridges::lua>>;
            /**
             * @brief Stateful Lua label using Redis session storage in immediate mode.
             */
            using immediate_redis   = www::basic_label<StreamT, www::tags::statefulx<udho::session::storage::redis, udho::session::modes::immediate, udho::view::data::bridges::lua>>;
#endif // WITH_HIREDIS
        };
    };

};


// }

}
}

#endif // UDHO_WWW_PRESETS_H
