#ifndef UDHO_UTILS_DATE_TIME_H
#define UDHO_UTILS_DATE_TIME_H

#include <string>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <mutex>

namespace udho{
namespace utils {

namespace date_time{

namespace detail{

/**
 * @brief Thread‑safe conversion of a time_t to a local time struct tm.
 * @param timer Time in seconds since the epoch.
 * @return struct tm representing the local time corresponding to @p timer.
 * @note Uses reentrant functions (`localtime_r` on Unix, `localtime_s` on Windows) where available.
 *       On other platforms it falls back to a mutex‑protected `std::localtime`.
 * @see https://stackoverflow.com/a/38034148
 */
inline struct std::tm localtime_xp(std::time_t timer) {
    struct std::tm bt {};
#if defined(__unix__)
    ::localtime_r(&timer, &bt);
#elif defined(_MSC_VER)
    ::localtime_s(&bt, &timer);
#else
    static std::mutex mtx;
    std::lock_guard<std::mutex> lock(mtx);
    bt = *std::localtime(&timer);
#endif
    return bt;
}

/**
 * @brief Thread‑safe conversion of a time_t to a UTC (GMT) struct tm.
 * @param timer Time in seconds since the epoch.
 * @return struct tm representing the UTC time corresponding to @p timer.
 * @note Uses reentrant functions (`gmtime_r` on Unix, `gmtime_s` on Windows) where available.
 *       On other platforms it falls back to a mutex‑protected `std::gmtime`.
 * @see https://stackoverflow.com/a/38034148
 */
inline struct std::tm gmtime_xp(std::time_t timer) {
    struct std::tm bt {};
#if defined(__unix__)
    ::gmtime_r(&timer, &bt);
#elif defined(_MSC_VER)
    ::gmtime_s(&bt, &timer);
#else
    static std::mutex mtx;
    std::lock_guard<std::mutex> lock(mtx);
    bt = *std::gmtime(&timer);
#endif
    return bt;
}

}

/**
 * @brief date time in required format
 * @details RFC 6265 section 4.1.1 specifies the cookie date format as follows
 *          sane-cookie-date  = <rfc1123-date, defined in [RFC2616], Section 3.3.1>
 *          RFC 2616 section-3.3.1 provides full specification as mentioned below
 *          However, RFC 2616 has been obsoleted by RFC 7231. The 7.1.1.1 of RFC 7231
 *          mentions the preferred format IMF-fixdate.
 * @param tp
 * @return
 */
inline std::string format_rfc7231(std::chrono::system_clock::time_point tp) {
    ::time_t t = std::chrono::system_clock::to_time_t(tp);
    struct std::tm tm = detail::gmtime_xp(t);

    std::ostringstream oss;
    oss.imbue(std::locale("C"));
    oss << std::put_time(&tm, "%a, %d %b %Y %H:%M:%S GMT");
    return oss.str();
}

/**
 * @brief Format a time_point according to RFC 3339 (with nanoseconds and time‑zone offset)
 * @details RFC 3339 defines the Internet date/time format: YYYY-MM-DDTHH:MM:SS[.frac]±HH:MM
 *          This function outputs nanoseconds (9 digits) and always includes the time‑zone
 *          offset in ±HH:MM format. The offset is computed from the system's local time
 *          zone at the given time point.
 * @param tp The time point to format
 * @return RFC 3339 string, e.g., "2025-04-01T12:34:56.123456789+02:00"
 */
inline std::string format_rfc3339(std::chrono::system_clock::time_point tp) {
    auto since_epoch    = tp.time_since_epoch();
    auto sec_floor      = std::chrono::floor<std::chrono::seconds>(since_epoch);
    auto frac_ns        = std::chrono::duration_cast<std::chrono::nanoseconds>(since_epoch - sec_floor);

    ::time_t utc = std::chrono::system_clock::to_time_t(tp);
    struct std::tm tm_local = detail::localtime_xp(utc);
    struct std::tm tm_gmt   = detail::gmtime_xp(utc);

    tm_local.tm_isdst = -1;
    ::time_t local_t = std::mktime(&tm_local);
    ::time_t gmt_t   = std::mktime(&tm_gmt);
    int offset_sec = static_cast<int>(std::difftime(local_t, gmt_t));

    std::string offset;
    if (offset_sec == 0) {
        offset = "Z";
    } else {
        char sign = (offset_sec > 0) ? '+' : '-';
        int abs_offset = std::abs(offset_sec);
        int hours = abs_offset / 3600;
        int minutes = (abs_offset % 3600) / 60;
        std::ostringstream off_oss;
        off_oss << sign << std::setw(2) << std::setfill('0') << hours
                << ':' << std::setw(2) << std::setfill('0') << minutes;
        offset = off_oss.str();
    }

    std::ostringstream oss;
    oss.imbue(std::locale("C"));
    oss << std::put_time(&tm_local, "%Y-%m-%dT%H:%M:%S")
        << '.' << std::setw(9) << std::setfill('0') << frac_ns.count()
        << offset;
    return oss.str();
}

/**
 * @brief Format a duration as an ISO 8601 duration string.
 *
 * Converts a std::chrono::duration to the ISO 8601 duration format:
 *     P[n]DT[n]H[n]M[n]S
 * where:
 *   - D: days (24-hour units)
 *   - T separates date and time parts
 *   - H, M, S: hours, minutes, seconds
 *   - S may include a decimal fraction (up to 9 digits, trailing zeros trimmed)
 *   - Zero duration is output as "PT0S"
 *   - Negative durations are prefixed with '-'
 *
 * @tparam Rep        Arithmetic type representing the number of ticks
 * @tparam Period     std::ratio representing the tick period
 * @param duration    The duration to format
 * @return std::string ISO 8601 duration string
 */
template<class Rep, class Period>
inline std::string format_iso8601(const std::chrono::duration<Rep, Period>& duration){
    bool negative    = duration < duration.zero();
    auto total       = std::chrono::abs(duration);
    auto total_hours = std::chrono::duration_cast<std::chrono::hours>(total);
    auto D           = total_hours.count() / 24;
    auto H           = total_hours.count() % 24;
    auto after_days  = total - std::chrono::hours(D * 24 + H);
    auto M           = std::chrono::duration_cast<std::chrono::minutes>(after_days).count();
    after_days      -= std::chrono::minutes(M);
    auto S           = std::chrono::duration_cast<std::chrono::seconds>(after_days).count();
    after_days      -= std::chrono::seconds(S);
    auto F           = std::chrono::duration_cast<std::chrono::nanoseconds>(after_days).count();

    std::ostringstream oss;
    if (negative) oss << '-';
    oss << 'P';

    bool has_date = (D > 0);
    bool has_time = (H > 0 || M > 0 || S > 0 || F > 0);

    if (has_date) oss << D << 'D';
    if (has_time) {
        oss << 'T';
        if (H > 0) oss << H << 'H';
        if (M > 0) oss << M << 'M';
        if (S > 0 || F > 0) {
            oss << S;
            if (F > 0) {
                // Format fractional part with up to 9 digits, trimming trailing zeros
                std::string frac = std::to_string(F);
                if (frac.size() < 9) frac = std::string(9 - frac.size(), '0') + frac;
                while (!frac.empty() && frac.back() == '0') frac.pop_back();
                oss << '.' << frac;
            }
            oss << 'S';
        }
    }

    // Zero duration
    if (!has_date && !has_time) oss << "T0S";

    return oss.str();
}


/**
 * @brief Parse RFC 7231 date string to time point
 * @param date_str Date string in RFC 7231 format
 * @return Corresponding system_clock time point
 * @throws std::invalid_argument for invalid formats
 */
inline bool parse_rfc7231(std::string_view date_str_view, std::chrono::system_clock::time_point& result) {
    struct std::tm tm = {};
    std::string date_str(date_str_view);
    std::istringstream iss(date_str);
    iss.imbue(std::locale("C"));
    iss >> std::get_time(&tm, "%a, %d %b %Y %H:%M:%S GMT");

    if(iss.fail()) return false;

#ifdef _WIN32
    // Windows UTC conversion
    tm.tm_isdst = -1;  // Let mktime determine DST
    ::time_t tt = ::_mkgmtime(&tm);
#else
    // POSIX UTC conversion
    ::time_t tt = ::timegm(&tm);
#endif

    if(tt == -1) return false;
    result = std::chrono::system_clock::from_time_t(tt);
    return true;
}

}

}
}

#endif // UDHO_UTILS_DATE_TIME_H
