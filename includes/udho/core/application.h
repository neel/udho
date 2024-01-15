#ifndef UDHO_CORE_APPLICATION_H
#define UDHO_CORE_APPLICATION_H

#include <udho/url/mount.h>

namespace udho{
namespace core{

template <typename DerivedT>
struct application{
    template <typename CharT, CharT... C>
    auto mount(udho::hazo::string::str<CharT, C...>&& name, const std::string& path) {
        return udho::url::mount(std::move(name), path, std::move(self().routes()));
    }
    auto root() {
        return udho::url::root(std::move(self().routes()));
    }

    DerivedT& self() { return static_cast<DerivedT&>(*this); }
    const DerivedT& self() const { return static_cast<const DerivedT&>(*this); }
};

}
}

#endif // UDHO_CORE_APPLICATION_H
