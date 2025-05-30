#ifndef UDHO_SESSION_STORAGE_DETAIL_H
#define UDHO_SESSION_STORAGE_DETAIL_H

#include <cstdint>
#include <udho/session/record.h>

namespace udho{
namespace session{
namespace storage{

namespace detail{
    static constexpr std::uint32_t SESSION_FILE_MAGIC = 0x53B0E79E;

    #pragma pack(push, 1)
    struct attr_meta{
        std::uint32_t offset;
        std::uint32_t key_len;
        std::uint32_t value_len;
    };
    struct record_preamble{
        std::uint32_t MAGIC = detail::SESSION_FILE_MAGIC;
        std::uint16_t VERSION = 1;
    };
    #pragma pack(pop)

    static constexpr auto MIN_SIZE = sizeof(record_preamble) + sizeof(udho::session::record_data::sessid_type) + sizeof(std::uint32_t);
}

}
}
}

#endif // UDHO_SESSION_STORAGE_DETAIL_H
