#ifndef UDHO_LOGGING_FORMATTER_H
#define UDHO_LOGGING_FORMATTER_H

#include <boost/log/core/record_view.hpp>
#include <boost/log/utility/formatting_ostream.hpp>
#include <boost/log/attributes/value_extraction.hpp>
#include <boost/log/expressions/message.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <udho/logging/params.h>
#include <udho/utils/date_time.h>

namespace boost::log{
inline formatting_ostream& operator<<(formatting_ostream& stream, const std::chrono::system_clock::time_point& tp ) {
    stream << udho::utils::date_time::format_rfc3339(tp);
    return stream;
}

template<class Rep, class Period>
inline formatting_ostream& operator<<(formatting_ostream& stream, const std::chrono::duration<Rep, Period>& duration) {
    stream << udho::utils::date_time::format_iso8601(duration);
    return stream;
}

}

namespace udho{
namespace logging{

/**
 * @brief Formatter for transported log records reconstructed in the consumer.
 *
 * Output form:
 * @code
 * <required fields> <: Key1(value1) Key2(value2) :>
 * @endcode
 *
 * The mandatory fields are emitted first. Supported optional fields are then
 * appended in fixed formatter order when present on the record.
 *
 * @note Optional fields are separated by spaces, not commas.
 * @note The optional wrapper is emitted only when at least one supported
 *       optional field is printed.
 */
struct formatter{
    explicit formatter(bool full = false): _full(full) {}

    /**
     * @brief Format a single log record into the supplied output stream.
     * @tparam CharT character type of the formatting stream
     * @param record Boost.Log record view to format
     * @param stream destination stream
     */
    template <typename RecordViewT, typename StreamT>
    void operator()(const RecordViewT& record, StreamT& stream) const {
        std::size_t total_count    = record.attribute_values().size();
        std::size_t required_count = make_required(record, stream);

        if(total_count > required_count) {
            stream << " <:";
            make_optional(record, stream);
            stream << " :>";
        }
    }

    /**
     * @brief Emit the mandatory part of the log line.
     * @tparam CharT character type of the formatting stream
     * @param record Boost.Log record view
     * @param stream destination stream
     * @return number of mandatory fields logically accounted for by this formatter
     *
     * The mandatory fields are expected to be present on every transported record.
     */
    template <typename RecordViewT, typename StreamT>
    std::size_t make_required(const RecordViewT& record, StreamT& stream) const {
        namespace params = udho::logging::params;

        auto line_id   = boost::log::extract<params::local_id::value_type>(udho::logging::names::local_id, record);
        auto process   = boost::log::extract<params::process::value_type>(udho::logging::names::process, record);
        auto thread    = boost::log::extract<params::thread::value_type>(udho::logging::names::thread, record);
        auto timestamp = boost::log::extract<params::timestamp::value_type>(udho::logging::names::timestamp, record);
        auto severity  = boost::log::extract<params::severity::value_type>(udho::logging::names::severity, record);
        auto subsystem = boost::log::extract<params::subsystem::value_type>(udho::logging::names::subsystem, record);
        auto function  = boost::log::extract<params::function::value_type>(udho::logging::names::function, record);
        auto file      = boost::log::extract<params::file::value_type>(udho::logging::names::file, record);
        auto line      = boost::log::extract<params::line::value_type>(udho::logging::names::line, record);
        auto message   = record[boost::log::expressions::smessage];

        stream << line_id.get() << " PID(" << process.get() << ") TID(0x" << std::hex << thread.get() << std::dec << ")"
               << " " << timestamp.get() << " "
               << "[" << severity.get()  << "] "
               << "{" << subsystem.get() << "} "
               << *message;

        if(_full) stream << " " << "at " << file << ":"<< line << " " << "from `" << function << "`";

        return 10; // including message
    }

    /**
     * @brief Append one optional field if it exists on the record.
     * @tparam T extracted attribute type
     * @tparam CharT character type of the formatting stream
     * @param record Boost.Log record view
     * @param stream destination stream
     * @param name attribute name
     * @param first set to @c true before the first optional field; updated by the function
     * @return @c true if the attribute existed and was emitted, otherwise @c false
     *
     * When the first optional field is emitted, the formatter may prepend
     * the separator text such as @c " with ".
     */
    template <typename T, typename RecordViewT, typename StreamT>
    static bool append_optional(const RecordViewT& record, StreamT& stream, const char* name) {
        auto value = boost::log::extract<T>(name, record);
        if (!value) return false;

        stream << " " << name << "(" << value.get() << ")";
        return true;
    }

    /**
     * @brief Emit all supported optional fields in formatter order.
     * @tparam CharT character type of the formatting stream
     * @param record Boost.Log record view
     * @param stream destination stream
     * @return @c true if at least one optional field was emitted, otherwise @c false
     *
     * Optional fields are rendered in @c key(value) form and separated by commas.
     */
    template <typename RecordViewT, typename StreamT>
    StreamT& make_optional(const RecordViewT& record, StreamT& stream) const {
        namespace params = udho::logging::params;

        append_optional<params::request_id::value_type::value_type>(record, stream, udho::logging::names::request_id);
        append_optional<params::flow_id::value_type::value_type>(record, stream, udho::logging::names::flow_id);
        append_optional<params::socket_id::value_type::value_type>(record, stream, udho::logging::names::socket_id);
        append_optional<params::session_id::value_type::value_type>(record, stream, udho::logging::names::session_id);
        append_optional<params::user_id::value_type::value_type>(record, stream, udho::logging::names::user_id);
        append_optional<params::client::value_type::value_type>(record, stream, udho::logging::names::client);
        append_optional<params::host::value_type::value_type>(record, stream, udho::logging::names::host);
        append_optional<params::method::value_type::value_type>(record, stream, udho::logging::names::method);
        append_optional<params::uri::value_type::value_type>(record, stream, udho::logging::names::uri);
        append_optional<params::route::value_type::value_type>(record, stream, udho::logging::names::route);
        append_optional<params::query::value_type::value_type>(record, stream, udho::logging::names::query);
        append_optional<params::agent::value_type::value_type>(record, stream, udho::logging::names::agent);
        append_optional<params::status_code::value_type::value_type>(record, stream, udho::logging::names::status_code);
        append_optional<params::bytes_sent::value_type::value_type>(record, stream, udho::logging::names::bytes_sent);
        append_optional<params::latency::value_type::value_type>(record, stream, udho::logging::names::latency);
        append_optional<params::retry_count::value_type::value_type>(record, stream, udho::logging::names::retry_count);

        return stream;
    }

private:
    bool _full;
};

}
}

#endif // UDHO_LOGGING_FORMATTER_H
