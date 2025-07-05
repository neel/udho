#ifndef UDHO_SESSION_FWD_H
#define UDHO_SESSION_FWD_H

#include <udho/session/defs.h>

namespace udho{
namespace session{

struct record_data;
struct record;
struct note;

template <typename StorageT, udho::session::modes Mode>
struct catalogue;

namespace storage{
    struct fs;
}

}
}

#endif // UDHO_SESSION_FWD_H
