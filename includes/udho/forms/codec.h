/*
 * Copyright (c) 2020, Neel Basu <neel.basu.z@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *     * Neither the name of the <organization> nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY Neel Basu <neel.basu.z@gmail.com> ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL Neel Basu <neel.basu.z@gmail.com> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef UDHO_FORMS_CODEC_H
#define UDHO_FORMS_CODEC_H

/**
 * @file
 * @brief Converts parsed form text to field values and back.
 */

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iterator>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>
#include <boost/lexical_cast.hpp>
#include <udho/net/protocols/form_data.h>
#include <udho/url/detail/function.h>
#include <udho/utils/date_time.h>

namespace udho{
namespace forms{

/// @brief Default format for local date-time fields.
inline constexpr const char* default_datetime_format = "%Y-%m-%d %H:%M:%S";

namespace detail{

/// @brief Parsed storage containing form fields and uploaded file entries.
using form_data = udho::net::protocols::detail::form_data;
/// @brief Non-owning lookup view over parsed form storage.
using form_view = udho::net::protocols::detail::form_view;

/// @brief Whether the decayed type is a supported parsed form or form view.
template <typename T>
inline constexpr bool is_form_v = std::is_same_v<std::decay_t<T>, form_data>
                               || std::is_same_v<std::decay_t<T>, form_view>;

/// @brief Create a lookup view; the parsed form must outlive it.
inline form_view view(const form_data& form){ return form_view(form); }
/// @brief Copy a lookup view without extending its underlying storage lifetime.
inline form_view view(const form_view& form){ return form; }

/**
 * @brief Converts a field value using the current URL converter and Boost lexical serialization.
 * @tparam T Parsed field value type.
 */
template <typename T>
struct value_codec{
    /// @brief Write the converted value and return whether parsing succeeded.
    static bool parse(const std::string& input, T& value){
        bool okay = false;
        value = udho::url::detail::convert_str_to_type<T>::apply(input, &okay);
        return okay;
    }
    /// @brief Serialize a value using Boost lexical conversion.
    static std::string serialize(const T& value){
        return boost::lexical_cast<std::string>(value);
    }
};

/**
 * @brief Parses and formats system-clock time points in local time.
 * @tparam DurationT Duration precision of the stored time point.
 */
template <typename DurationT>
struct value_codec<std::chrono::time_point<std::chrono::system_clock, DurationT>>{
    /// @brief Time point produced by this codec.
    using value_type = std::chrono::time_point<std::chrono::system_clock, DurationT>;

    // As in the original API, date-time fields use local time.
    /**
     * @brief Parse local time, rejecting trailing non-whitespace and calendar normalization.
     * @param[out] value Parsed time point; unchanged if parsing fails.
     * @return Whether the input matches the format and represents the parsed local time.
     */
    static bool parse(const std::string& input, value_type& value,
                      const std::string& format = default_datetime_format){
        std::tm tm{};
        tm.tm_mday = 1;
        tm.tm_isdst = -1;
        std::istringstream stream(input);
        stream >> std::get_time(&tm, format.c_str());
        if(stream.fail()) return false;
        stream >> std::ws;
        if(!stream.eof()) return false;
        const auto parsed = tm;
        const auto time = std::mktime(&tm);
        const auto normalized = udho::utils::date_time::detail::localtime_xp(time);
        if(normalized.tm_year != parsed.tm_year || normalized.tm_mon != parsed.tm_mon
           || normalized.tm_mday != parsed.tm_mday || normalized.tm_hour != parsed.tm_hour
           || normalized.tm_min != parsed.tm_min || normalized.tm_sec != parsed.tm_sec){
            return false;
        }
        value = std::chrono::time_point_cast<DurationT>(std::chrono::system_clock::from_time_t(time));
        return true;
    }
    /// @brief Format a time point as local calendar time using the supplied format.
    static std::string serialize(const value_type& value,
                                 const std::string& format = default_datetime_format){
        const auto time = std::chrono::system_clock::to_time_t(
            std::chrono::time_point_cast<std::chrono::system_clock::duration>(value));
        const auto tm = udho::utils::date_time::detail::localtime_xp(time);
        std::ostringstream stream;
        stream << std::put_time(&tm, format.c_str());
        return stream.str();
    }
};

// A scalar field reads the first occurrence. Upload paths and buffers are not text fields.
/**
 * @brief Parse the first occurrence of a named field when it contains nonempty text.
 * @param form Parsed form storage or a lookup view over it.
 * @param name Field name to look up.
 * @param[out] okay True only when the selected text parses successfully.
 * @param args Extra codec arguments, such as a date-time format.
 * @return Parsed value, or T{} for absent, empty, non-text, or malformed input.
 * @details Uploaded file paths and buffers are not treated as text fields.
 */
template <typename T, typename FormT, typename... ArgsT>
T read_field(const FormT& form, const std::string& name, bool& okay, const ArgsT&... args){
    const auto fields = view(form);
    T value{};
    okay = false;
    if(fields.count(name) != 0){
        const auto& input = fields.field(name);
        if(input.is_string() && !input.string().empty()){
            okay = value_codec<T>::parse(input.string(), value, args...);
            if(!okay) value = T{};
        }
    }
    return value;
}

}

}
}

#endif // UDHO_FORMS_CODEC_H
