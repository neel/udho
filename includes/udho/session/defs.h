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

using time_point     = std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds>;
using duration_type  = time_point::duration;

}
}

#endif // UDHO_SESSION_DEFS_H
