#ifndef UDHO_UTILS_DATE_TIME_H
#define UDHO_UTILS_DATE_TIME_H

#include <string>
#include <chrono>
#include <sstream>
#include <iomanip>

namespace udho{
namespace utils {

namespace date_time{


/**
 * @brief date time in required format
 * @details RFC 6265 section 4.1.1 specifies the cookie date format as follows
 *          sane-cookie-date  = <rfc1123-date, defined in [RFC2616], Section 3.3.1>
 *          RFC 2616 section-3.3.1 provides full specification as mentioned below
 *          However, RFC 2616 has been obsoleted by RFC 7231. The 7.1.1.1 of RFC 7231
 *          mentions the preffered format IMF-fixdate.
 * @param tp
 * @return
 */
inline std::string format_rfc7231(std::chrono::system_clock::time_point tp) {
    std::time_t t = std::chrono::system_clock::to_time_t(tp);
    std::tm tm;

#ifdef _WIN32
    gmtime_s(&tm, &t);
#else
    gmtime_r(&t, &tm);
#endif

    // Force English locale for day/month abbreviations
    std::ostringstream oss;
    oss.imbue(std::locale("C"));

    const std::time_put<char>& facet = std::use_facet<std::time_put<char>>(oss.getloc());

    char pattern[] = "%a, %d %b %Y %H:%M:%S GMT";
    char* pat_end = pattern + sizeof(pattern) - 1;

    facet.put(oss, oss, ' ', &tm, pattern, pat_end);

    return oss.str();
}

/**
 * @brief Parse RFC 7231 date string to time point
 * @param date_str Date string in RFC 7231 format
 * @return Corresponding system_clock time point
 * @throws std::invalid_argument for invalid formats
 */
inline bool parse_rfc7231(std::string_view date_str_view, std::chrono::system_clock::time_point& result) {
    std::tm tm = {};
    std::string date_str(date_str_view);
    std::istringstream iss(date_str);
    iss.imbue(std::locale("C"));
    iss >> std::get_time(&tm, "%a, %d %b %Y %H:%M:%S GMT");

    if(iss.fail()) return false;

#ifdef _WIN32
    // Windows UTC conversion
    tm.tm_isdst = -1;  // Let mktime determine DST
    time_t tt = _mkgmtime(&tm);
#else
    // POSIX UTC conversion
    time_t tt = timegm(&tm);
#endif

    if(tt == -1) return false;
    result = std::chrono::system_clock::from_time_t(tt);
    return true;
}

}

}
}

#endif // UDHO_UTILS_DATE_TIME_H
