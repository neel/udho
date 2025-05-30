#ifndef UDHO_SESSION_STORAGE_DETAIL_H
#define UDHO_SESSION_STORAGE_DETAIL_H

#include <cstdint>
#include <udho/session/record_data.h>

namespace udho{
namespace session{
namespace storage{

namespace detail{
    static constexpr std::uint32_t SESSION_FILE_MAGIC = 0x4F484455;

    #pragma pack(push, 1)
    struct attr_meta{
        std::uint32_t offset;
        std::uint32_t key_len;
        std::uint32_t value_len;
    };
    struct record_preamble{
        using time_type = udho::session::record_data::time_point;
        using duration_type = typename time_type::duration;

        std::uint32_t MAGIC = detail::SESSION_FILE_MAGIC;
        std::uint16_t VERSION = 1;
        std::uint64_t created = 0;
        std::uint64_t updated = 0;

        time_type created_at() const { return time_type{duration_type{created}}; }
        time_type updated_at() const { return time_type{duration_type{updated}}; }

        void created_at(time_type time) {
            duration_type since_epoch = time.time_since_epoch();
            created = since_epoch.count();
        }

        void update() {
            time_type current_time = std::chrono::system_clock::now();
            duration_type since_epoch = current_time.time_since_epoch();
            updated = since_epoch.count();
        }
    };
    #pragma pack(pop)

    static_assert(sizeof(record_preamble) == 4+2+8+8, "preamble must be exactly 22 bytes");

    static constexpr auto MIN_SIZE = sizeof(record_preamble) + sizeof(udho::session::record_data::sessid_type) + sizeof(std::uint32_t);
}

}
}
}

#endif // UDHO_SESSION_STORAGE_DETAIL_H
