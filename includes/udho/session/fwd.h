#ifndef UDHO_SESSION_FWD_H
#define UDHO_SESSION_FWD_H

namespace udho{
namespace session{

struct record_data;
struct record;
struct note;

template <typename StorageT>
struct catalogue;

namespace storage{
    struct fs;
}

}
}

#endif // UDHO_SESSION_FWD_H
