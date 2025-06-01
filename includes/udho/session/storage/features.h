#ifndef UDHO_SESSION_STORAGE_FEATURES_H
#define UDHO_SESSION_STORAGE_FEATURES_H

#include <udho/session/defs.h>

namespace udho{
namespace session{
namespace storage{

template <modes... M>
struct features {
    static constexpr modes values[] = { M... };

    static constexpr bool has(modes m) {
        for (modes x : values) {
            if (x == m) return true;
        }
        return false;
    }
};

}
}
}

#endif // UDHO_SESSION_STORAGE_FEATURES_H
