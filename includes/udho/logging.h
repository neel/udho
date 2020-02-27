/*
 * Copyright (c) 2020, <copyright holder> <email>
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
 * THIS SOFTWARE IS PROVIDED BY <copyright holder> <email> ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <copyright holder> <email> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef UDHO_LOGGING_H
#define UDHO_LOGGING_H

#include <ostream>
#include <chrono>
#include <iomanip>
#include <boost/format.hpp>

namespace udho{
    
namespace logging{
    
enum class segment{
    unknown,
    router,
    server
};

enum class status{
    error,
    warning,
    info,
    debug
};

}
    
namespace loggers{
 
template <typename StreamT>
struct plain{
    StreamT& _stream;
    
    plain(StreamT& stream): _stream(stream){}
    void log(udho::logging::status status, udho::logging::segment segment, const std::string& message){
        std::time_t time = std::time(nullptr);
        std::tm tm = *std::localtime(&time);
        
        std::string status_str = "unknown";
        switch(status){
            case udho::logging::status::error:
                status_str = "error";
                break;
            case udho::logging::status::warning:
                status_str = "warning";
                break;
            case udho::logging::status::info:
                status_str = "info";
                break;
            case udho::logging::status::debug:
                status_str = "debug";
                break;
        }
        
        std::string segment_str = "unknown";
        switch(segment){
            case udho::logging::segment::unknown:
                segment_str = "unknown";
                break;
            case udho::logging::segment::router:
                segment_str = "router";
                break;
            case udho::logging::segment::server:
                segment_str = "server";
                break;
        }
        
        _stream << boost::format("%1% > [%2%] (%3%) %4%") % std::put_time(&tm, "%T") % status_str % segment_str % message << std::endl;
    }    
};

typedef plain<std::ostream> ostream;
    
}
}

#endif // UDHO_LOGGING_H
