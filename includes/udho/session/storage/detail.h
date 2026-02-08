#ifndef UDHO_SESSION_STORAGE_DETAIL_H
#define UDHO_SESSION_STORAGE_DETAIL_H

#include <cstdint>
#include <udho/session/defs.h>

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
        using time_type = udho::session::time_point;
        using duration_type = typename time_type::duration;

        std::uint32_t MAGIC    = detail::SESSION_FILE_MAGIC;
        std::uint16_t VERSION  = 1;
        std::uint8_t  created[8];
        std::uint8_t  updated[8];
        std::uint8_t  revision[8];

        static std::uint64_t load_u64(const std::uint8_t (&bytes)[8]) noexcept {
            std::uint64_t value;
            std::memcpy(&value, bytes, sizeof(value));
            return value;
        }

        static void store_u64(std::uint8_t (&bytes)[8], std::uint64_t value) noexcept {
            std::memcpy(bytes, &value, sizeof(value));
        }

        time_type created_at() const { return time_type{duration_type{load_u64(created)}}; }
        time_type updated_at() const { return time_type{duration_type{load_u64(updated)}}; }

        std::uint64_t get_revision() const { return load_u64(revision); }

        void set_revision(std::uint64_t rev) {
            store_u64(revision, rev);
        }

        void created_at(time_type time) {
            duration_type since_epoch = time.time_since_epoch();
            store_u64(created, since_epoch.count());
        }

        void update(time_type time) {
            duration_type since_epoch = time.time_since_epoch();
            store_u64(updated, since_epoch.count());
        }
        void update() {
            update(std::chrono::system_clock::now());
        }
    };
    #pragma pack(pop)

    static_assert(sizeof(record_preamble) == 4+2+8+8+8, "preamble must be exactly 30 bytes");

    static constexpr auto MIN_SIZE = sizeof(record_preamble) + sizeof(udho::session::id) + sizeof(std::uint32_t);
}

}
}
}

#endif // UDHO_SESSION_STORAGE_DETAIL_H
