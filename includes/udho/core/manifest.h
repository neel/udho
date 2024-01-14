#ifndef UDHO_CORE_MANIFEST_H
#define UDHO_CORE_MANIFEST_H

#include <udho/url/router.h>

namespace udho{
namespace core{

template <typename DerivedT>
struct manifest{
    const auto& router(){
        static auto router = udho::url::router(std::move(self().routes()));
        return router;
    }

    DerivedT& self() { return static_cast<DerivedT&>(*this); }
    const DerivedT& self() const { return static_cast<const DerivedT&>(*this); }
};

}
}


#endif // UDHO_CORE_MANIFEST_H
