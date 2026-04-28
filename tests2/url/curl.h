#ifndef UDHO_TESTS_CURL_H
#define UDHO_TESTS_CURL_H

#include <curl/curl.h>
#include <string>
#include <map>
#include <vector>
#include <boost/algorithm/string.hpp>

static size_t curl_writef(void *contents, size_t size, size_t nmemb, void *userp){
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

struct http_results{
    long code;
    std::string   body;
    std::map<std::string, std::string> headers;
};

http_results curl_fetch(CURL* curl, const std::string method, const std::string& url){
    CURLcode res;
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method.c_str());
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, "http");
    struct curl_slist *headers = NULL;
    std::string response_headers;
    std::string response_body;
    std::map<std::string, std::string> headers_map;
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER,      headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,   curl_writef);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION,  curl_writef);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA,      &response_headers);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,       &response_body);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT,         10L);
    res = curl_easy_perform(curl);
    long response_code = 0;
    if(res == CURLE_OK) {
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        std::vector<std::string> header_lines;
        boost::algorithm::split(header_lines, response_headers, boost::is_any_of("\r\n"));
        for(const std::string& line: header_lines){
            std::vector<std::string> header_parts;
            boost::algorithm::split(header_parts, line, boost::is_any_of(":"));
            if(header_parts.size() >= 2){
                headers_map.insert(std::make_pair(boost::algorithm::trim_copy(header_parts[0]), boost::algorithm::trim_copy(header_parts[1])));
            }
        }
    }
    return http_results{response_code, response_body, headers_map};
}


#endif // UDHO_TESTS_CURL_H
