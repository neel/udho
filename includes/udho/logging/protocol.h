#ifndef UDHO_LOGGING_PROTOCOL_H
#define UDHO_LOGGING_PROTOCOL_H

#include <cstdint>
#include <algorithm>
#include <type_traits>

namespace udho {
namespace logging {

namespace protocol {

constexpr std::uint64_t MAGIC = 0xDEADBEEFCAFEBABEull;

enum class command : std::uint32_t {
    filter_set = 1,
    filter_unset,
    filter_show,

    threshold_set,
    threshold_show,

    sink_list,
    sink_add,
    sink_remove
};

struct request_header {
    std::uint64_t magic;   // must be MAGIC
    std::uint32_t cmd;     // command enum
    std::uint32_t length;  // size of payload (0..MAX_PAYLOAD)

    request_header() = default;
    inline request_header(command c, std::uint32_t len): magic(MAGIC), cmd(static_cast<std::uint32_t>(c)), length(len) {}
};

static_assert(sizeof(request_header) == 16, "header must be 16 bytes");

struct reply_header {
    std::uint64_t magic;
    std::uint32_t length;
    std::uint8_t  success;
    std::uint8_t  reserved[3];

    reply_header() = default;
    inline reply_header(bool s, std::uint32_t len):  magic(MAGIC), length(len), success(s) {
        std::fill_n(reserved, 0, 3);
    }
};

static_assert(std::is_trivially_copyable<reply_header>::value, "reply_header must be trivially copyable");
static_assert(sizeof(reply_header) == 16, "unexpected reply_header size");

}


}
}

#endif // UDHO_LOGGING_PROTOCOL_H
