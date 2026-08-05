#ifndef UDHO_SESSION_DEFS_H
#define UDHO_SESSION_DEFS_H

#include <chrono>
#include <type_traits>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/string_generator.hpp>

namespace udho{
namespace session{

/** @addtogroup DoxyG_session
 *  @{
 */

using id = boost::uuids::uuid;

static_assert(std::is_trivially_copyable<udho::session::id>::value);
static_assert(std::is_standard_layout<udho::session::id>::value);

inline std::string to_string(const session::id& id) {
    return boost::uuids::to_string(id);
}

inline session::id from_string(const std::string& str) {
    boost::uuids::string_generator gen;
    return gen(str);
}

inline session::id random() {
    boost::uuids::random_generator uuid_generator;
    return uuid_generator();
}

inline bool is_nill(const session::id& sessid) {
    return sessid.is_nil();
}

using time_point     = std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds>;
using duration_type  = time_point::duration;

/**
 * @enum modes
 * @brief Session write‐policy modes for catalogue implementations.
 *
 * This enum specifies four different session‐save behaviors:
 *  - @c none:        No automatic saving; user must manually invoke save.
 *  - @c lazy:        Classic write‐back: session is written only when
 *                    the last in‐memory “note” is destroyed.
 *  - @c optimistic:  Lazy + CAS (optimistic concurrency):
 *                    uses a version check before writing to detect conflicts.
 *  - @c immediate:   Write‐through: every @c set(...) calls save() immediately.
 */
enum class modes{
    /**
     * @brief No automatic saving is performed.
     */
    none,
    /**
     * @brief Lazy write‐back mode.
     *
     * The session is only persisted when the last borrower (e.g., @c note
     * object) goes out of scope and its destructor is invoked. If multiple
     * @c note instances exist, destruction of inner ones does not trigger
     * a save. This is the classic write‐back cache pattern.
     */
    lazy,
    /**
     * @brief Lazy write‐back with optimistic concurrency (CAS).
     *
     * Works like @c lazy for when to write, but before performing the write,
     * an atomic Compare‐And‐Swap (CAS) check on the session’s version/revision
     * is done. If the on‐disk revision does not match the in‐memory revision,
     * a conflict exception (@c udho::session::errors::conflict) is thrown,
     * preventing unintentional overwrites in distributed or multi‐process setups.
     */
    optimistic,
    /**
     * @brief Immediate write‐through mode.
     *
     * Every time @c set(...) (or @c unset(...)) is called on a @c note, the
     * storage backend’s save() is invoked immediately. This ensures the
     * on‐disk (or remote) session state is always up to date, at the cost of
     * higher I/O overhead per session mutation.
     */
    immediate
};

/** @} */
}
}

#endif // UDHO_SESSION_DEFS_H
