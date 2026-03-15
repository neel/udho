#ifndef UDHO_LOGGING_MESSAGE_H
#define UDHO_LOGGING_MESSAGE_H

#include <vector>
#include <nlohmann/json.hpp>
#include <udho/hazo/map.h>
#include <udho/logging/serialization.h>
#include <udho/logging/params.h>

namespace udho {
namespace logging {

template <typename... Fields>
struct schema: udho::hazo::map_d<Fields...>{
    using map_type      = udho::hazo::map_d<Fields...>;
    using buffer_type   = std::vector<std::uint8_t>;

    using map_type::map_type;

    buffer_type save() const {
        buffer_type buffer;
        buffer.reserve(byte_size());

        std::uint32_t len = 0;
        auto len_begin = reinterpret_cast<const std::uint8_t*>(&len);
        auto len_end   = len_begin + sizeof(std::uint32_t);
        buffer.insert(buffer.end(), len_begin, len_end);

        map_type::visit(detail::binary_serializer{buffer});

        len = buffer.size() - sizeof(std::uint32_t);
        std::memcpy(buffer.data(), &len, sizeof(len));
        return buffer;
    }

    bool load(const std::uint8_t* buffer, std::size_t buffer_size) {
        if (buffer_size < sizeof(std::uint32_t)) return false;

        std::uint32_t len;
        std::memcpy(&len, buffer, sizeof(std::uint32_t));

        if (len > buffer_size - sizeof(std::uint32_t)) return false;

        const std::uint8_t* begin = buffer +sizeof(std::uint32_t);
        const std::uint8_t* end   = begin  +len;

        try {
            map_type::visit(detail::binary_deserializer(begin, len));
        } catch (const std::invalid_argument&) {
            return false;
        }

        if (begin != end) {
            return false;
        }
        return true;
    }

    template <typename Function>
    void visit(Function&& f) {
        map_type::visit(std::forward<Function>(f));
    }

    template <typename Function>
    void visit(Function&& f) const {
        map_type::visit(std::forward<Function>(f));
    }

    std::size_t byte_size() const {
        std::size_t total = sizeof(std::uint32_t);
        map_type::visit(detail::binary_size_calculator{total});
        return total;
    }

    static std::size_t min_size() {
        std::size_t size = sizeof(std::uint32_t);
        map_type map;
        map.visit(detail::binary_size_calculator(size));
        return size;
    }

private:
    static std::size_t _min_size;
};
template <typename... Fields>
std::size_t schema<Fields...>::_min_size = schema<Fields...>::min_size();

using message = schema<
    udho::logging::params::local_id,
    udho::logging::params::timestamp,
    udho::logging::params::severity,
    udho::logging::params::thread,
    udho::logging::params::process,
    udho::logging::params::subsystem,
    udho::logging::params::message,
    udho::logging::params::file,
    udho::logging::params::function,
    udho::logging::params::line,

    udho::logging::params::request_id,
    udho::logging::params::flow_id,
    udho::logging::params::session_id,
    udho::logging::params::user_id,

    udho::logging::params::client_ip,
    udho::logging::params::host,
    udho::logging::params::http_method,
    udho::logging::params::uri,
    udho::logging::params::route,
    udho::logging::params::query,
    udho::logging::params::user_agent,

    udho::logging::params::status_code,
    udho::logging::params::bytes_sent,
    udho::logging::params::latency,
    udho::logging::params::retry_count,

    udho::logging::params::error_code,
    udho::logging::params::error_message
>;

}
}

#endif // UDHO_LOGGING_MESSAGE_H
