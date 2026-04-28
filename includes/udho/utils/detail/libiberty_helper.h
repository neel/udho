#ifndef UDHO_UTILS_LIBIBERTY_HELPER_H
#define UDHO_UTILS_LIBIBERTY_HELPER_H

#ifdef WITH_LIBIBERTY

#include <string>
#include <memory>

#pragma push_macro("HAVE_DECL_BASENAME")
#ifndef HAVE_DECL_BASENAME
#define HAVE_DECL_BASENAME 1
#endif

extern "C" {
#include <libiberty/demangle.h>
}

#pragma pop_macro("HAVE_DECL_BASENAME")

namespace udho{
namespace utils{
namespace detail{

static std::string demangle_with_libiberty(const std::string& symbol) {
    if(symbol.empty()) {
        return {};
    }

    int flags =
        DMGL_PARAMS  |
        DMGL_ANSI    |
        DMGL_TYPES   |
        DMGL_VERBOSE |
        DMGL_GNU_V3  |
        DMGL_NO_RECURSE_LIMIT;

    std::unique_ptr<char, void(*)(void*)> out(
        cplus_demangle(symbol.c_str(), flags),
        std::free
    );

    if(out) {
        return std::string(out.get());
    }

    return symbol;
}

}
}
}

#endif // WITH_LIBIBERTY

#endif // UDHO_UTILS_LIBIBERTY_HELPER_H
