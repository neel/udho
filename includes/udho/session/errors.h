#ifndef UDHO_SESSION_ERRORS_H
#define UDHO_SESSION_ERRORS_H

#include <string>
#include <cstdint>
#include <exception>
#include <udho/utils/format.h>
#include <udho/utils/filesystem.h>
#include <udho/session/defs.h>

namespace udho{
namespace session{
namespace errors{

/** @addtogroup DoxyG_session
 *  @{
 */

class conflict: std::exception{
    std::string   _message;
    udho::session::id _id;
    std::uint64_t _expected;
    std::uint64_t _observed;
    public:
        inline explicit conflict(udho::session::id id, std::uint64_t expected, std::uint64_t observed): _id(id), _expected(expected), _observed(observed) {
            _message = udho::utils::format(
                "version conflict while saving session {}; expected revision {}, observed revision {}",
                udho::session::to_string(_id), _expected, _observed
            );
        }

        inline explicit conflict(udho::session::id id, std::uint64_t expected, std::uint64_t observed, const std::string& message): _id(id), _expected(expected), _observed(observed), _message(message) {}

        inline udho::session::id id() const { return _id; }

        inline std::uint64_t expected() const { return _expected; }
        inline std::uint64_t observed() const { return _observed; }

        inline conflict& message(const std::string& msg) { _message = msg; return *this; }
        inline const std::string& message() const { return _message; }
        virtual const char* what() const noexcept { return _message.c_str(); }
};

class corruption: std::exception{
public:
    enum class type { unknown, magic_invalid, unsupported_version, too_small, invalid_offset, sessid_mismatch };
private:
    corruption::type          _type;
    std::string   _message;
    udho::session::id _id;
public:
    inline explicit corruption(type t, const std::string& message): _type(t), _message(message) {}
    inline explicit corruption(type t) {
        if(t == corruption::type::magic_invalid){
            _message = "Magic didn't match";
        } else if(t == corruption::type::unsupported_version) {
            _message = "Unsupported file version";
        } else if (t == corruption::type::too_small) {
            _message = "Corrupt file: too small";
        } else if (t == corruption::type::invalid_offset) {
            _message = "Corrupt file: attribute out of bounds";
        } else if (t == corruption::type::sessid_mismatch) {
            _message = "Session ID mismatch";
        } else {
            _message = "Unknown corruption";
        }
    }

    static inline corruption magic_invalid() { return corruption{type::magic_invalid}; }
    static inline corruption unsupported_version() { return corruption{type::unsupported_version}; }
    static inline corruption too_small() { return corruption{type::too_small}; }
    static inline corruption invalid_offset() { return corruption{type::invalid_offset}; }
    static inline corruption sessid_mismatch() { return corruption{type::sessid_mismatch}; }

    inline corruption::type type() const { return _type; }

    inline corruption& message(const std::string& msg) { _message = msg; return *this; }
    inline const std::string& message() const { return _message; }
    virtual const char* what() const noexcept { return _message.c_str(); }
};

class io: std::exception{
    udho::utils::filesystem::path _path;
    std::string _message;

    public:
        inline explicit io(const udho::utils::filesystem::path& path, const std::string& message): _path(path), _message(message) {}

        inline const udho::utils::filesystem::path& path() const { return _path; }

        inline io& message(const std::string& msg) { _message = msg; return *this; }
        inline const std::string& message() const { return _message; }
        virtual const char* what() const noexcept { return _message.c_str(); }
};

/** @} */

}
}
}

#endif // UDHO_SESSION_ERRORS_H
