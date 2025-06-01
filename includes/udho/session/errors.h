#ifndef UDHO_SESSION_ERRORS_H
#define UDHO_SESSION_ERRORS_H

#include <string>
#include <cstdint>
#include <exception>
#include <udho/utils/format.h>
#include <udho/session/defs.h>

namespace udho{
namespace session{
namespace errors{

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

}
}
}

#endif // UDHO_SESSION_ERRORS_H
