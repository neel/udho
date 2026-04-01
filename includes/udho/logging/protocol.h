#ifndef UDHO_LOGGING_PROTOCOL_H
#define UDHO_LOGGING_PROTOCOL_H

#include <cstdint>
#include <algorithm>
#include <type_traits>

namespace udho {
namespace logging {

namespace protocol {

/**
 * @brief Magic constant used to identify valid admin protocol packets.
 *
 * All request and reply headers must carry this value in their @c magic field.
 * Packets with any other value are treated as malformed.
 */
constexpr std::uint64_t MAGIC = 0xDEADBEEFCAFEBABEull;

/**
 * @brief Administrative command identifiers understood by the logging consumer.
 *
 * These commands are transported over the Unix-domain admin socket.
 * Each command may optionally carry a payload whose layout depends on the command.
 */
enum class command : std::uint32_t {
    filter_set = 1,
    filter_unset,
    filter_show,
    temporary_enable
};

/**
 * @brief Fixed-size header that prefixes every admin request packet.
 *
 * A request packet consists of this header followed by exactly @c length bytes
 * of payload. The payload may be empty for commands that do not require one.
 *
 * Packet layout:
 * - @c magic  : must equal @ref MAGIC
 * - @c cmd    : numeric value of @ref command
 * - @c length : payload size in bytes
 */
struct request_header {
    std::uint64_t magic;   // must be MAGIC
    std::uint32_t cmd;     // command enum
    std::uint32_t length;  // size of payload (0..MAX_PAYLOAD)

    /**
     * @brief Default constructor.
     *
     * Leaves the object default-initialized.
     */
    request_header() = default;

    /**
     * @brief Construct a valid request header for the specified command.
     * @param c command identifier
     * @param len payload size in bytes
     */
    inline request_header(command c, std::uint32_t len): magic(MAGIC), cmd(static_cast<std::uint32_t>(c)), length(len) {}
};

static_assert(sizeof(request_header) == 16, "header must be 16 bytes");

/**
 * @brief Fixed-size header that prefixes every admin reply packet.
 *
 * A reply packet consists of this header followed by exactly @c length bytes
 * of reply text or reply payload.
 *
 * Packet layout:
 * - @c magic   : must equal @ref MAGIC
 * - @c length  : reply payload size in bytes
 * - @c success : nonzero for success, zero for failure
 * - @c reserved: reserved for future use, currently zeroed
 */
struct reply_header {
    std::uint64_t magic;
    std::uint32_t length;
    std::uint8_t  success;
    std::uint8_t  reserved[3];

    /**
     * @brief Default constructor.
     *
     * Leaves the object default-initialized.
     */
    reply_header() = default;

    /**
     * @brief Construct a valid reply header.
     * @param s success flag
     * @param len reply payload size in bytes
     */
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
