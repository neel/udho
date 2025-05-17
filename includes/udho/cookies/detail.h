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

#ifndef UDHO_COOKIES_DETAIL_H
#define UDHO_COOKIES_DETAIL_H

#include <sstream>
#include <locale>
#include <string>
#include <boost/algorithm/string.hpp>
#include <udho/cookies/policy.h>
#include <chrono>
#include <iomanip>

namespace udho{
namespace cookies{
namespace detail{

inline std::string_view trim_view(std::string_view s) {
    auto front = s.find_first_not_of(" \t");
    if (front == std::string_view::npos) return "";
    auto back = s.find_last_not_of(" \t");
    return s.substr(front, back - front + 1);
}

inline bool parse_unsigned(std::string_view s, unsigned long& result) {
    std::string str{s};
    try{
        result = std::stoul(str);
        return true;
    } catch (const std::exception& ex){
        return false;
    }

}

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

inline std::string same_site_str(udho::cookies::policy p){
    switch(p) {
        case udho::cookies::policy::none:   return "None";
        case udho::cookies::policy::strict: return "Strict";
        case udho::cookies::policy::lax:    return "Lax";
    }
    return "";
}

/**
 * @brief Parse SameSite policy string
 * @param policy_str Policy string
 * @return Corresponding policy enum
 */
inline bool parse_policy(std::string_view policy_str, udho::cookies::policy& p) {
    if(boost::algorithm::iequals(policy_str, "None")) {
        p = udho::cookies::policy::none;
        return true;
    }
    else if(boost::algorithm::iequals(policy_str, "Lax")) {
        p = udho::cookies::policy::lax;
        return true;
    }
    else if(boost::algorithm::iequals(policy_str, "Strict")) {
        p = udho::cookies::policy::strict;
        return true;
    }
    return false;
}

}
}
}

#endif // UDHO_COOKIES_DETAIL_H
