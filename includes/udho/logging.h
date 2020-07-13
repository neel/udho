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
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/date_time/posix_time/posix_time_io.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <udho/termcolor.hpp>

namespace udho{
    
namespace logging{
    
enum class segment{
    unknown,
    router,
    server,
    parser,
    user
};

enum class status{
    error,
    warning,
    info,
    debug
};

/**
 * A logging message
 * \tparam Status
 */
template <udho::logging::status Status>
struct message{
    typedef std::chrono::system_clock::time_point time_type;
    
    static constexpr const udho::logging::status status = Status;
    const time_type time = std::chrono::system_clock::now();
    
    const std::string _segment;
    const unsigned    _level;
    std::string _content;
    
    message(const std::string& segment, unsigned level = 0): _segment(segment), _level(level){}
    message(const std::string& segment, const std::string& content, unsigned level = 0): _segment(segment), _content(content), _level(level){}
    
    virtual std::string what() const{
        return _content;
    }
};

template <typename StreamT, udho::logging::status Status>
StreamT& operator<<(StreamT& stream, const message<Status>& msg){
    stream << msg.what();
    return stream;
}

namespace messages{
    typedef message<udho::logging::status::error> error;
    typedef message<udho::logging::status::warning> warning;
    typedef message<udho::logging::status::info> info;
    typedef message<udho::logging::status::debug> debug;
    
    // std::cout << udho::logging::messages::formatted::error("nowhere", "Hello %1% see, %2% + %3% = %4%") << "Neel" << 2 << 3.2 << 5.2 << std::endl;
    namespace formatted{
        template <udho::logging::status Status>
        struct message: udho::logging::message<Status>{
            typedef messages::formatted::message<Status> self_type;
            typedef udho::logging::message<Status> base_type;
            
            boost::format _format;
            message(const std::string& segment, const std::string& format, unsigned level = 0): base_type(segment, level), _format(format){}
            template <typename T>
            self_type& prepare(const T& value){
                _format % value;
                return *this;
            }
            template <typename T>
            self_type& operator%(const T& value){
                return prepare(value);
            }
            virtual std::string what() const{
                return boost::str(_format);
            }
        };
        
        template <udho::logging::status Status, typename T>
        message<Status>& operator<<(message<Status>& msg, const T& value){
            msg % value;
            return msg;
        }
        
        typedef message<udho::logging::status::error> error;
        typedef message<udho::logging::status::warning> warning;
        typedef message<udho::logging::status::info> info;
        typedef message<udho::logging::status::debug> debug;
        
    }
}

namespace features{

struct colored{
    bool _colored;
    
    inline explicit colored(): _colored(false){}
    inline explicit colored(bool c): _colored(c){}
};
    
}

}
    
namespace loggers{
    
 
template <typename StreamT>
struct plain{
    typedef StreamT stream_type;
    typedef plain<StreamT> self_type;
    
    StreamT& _stream;
    logging::features::colored _feature_color;
    
    plain(StreamT& stream): _stream(stream){}
    
    void log(const std::chrono::system_clock::time_point& time, udho::logging::status status, const std::string& segment, int level, const std::string& message){
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
        
        boost::posix_time::ptime now = boost::posix_time::second_clock::local_time();
        
        if(!_feature_color._colored){
            _stream << boost::format("%1% > [%2%] (%3%) %4%") % now % status_str % segment % message << std::endl;
        }else{
            _stream << now;
            _stream << " ";
            _stream << ">>";
            _stream << " ";
            _stream << "(" << segment << ")";
            _stream << " ";
            if(status == udho::logging::status::error){
                _stream << termcolor::red;
            }else if(status == udho::logging::status::warning){
                _stream << termcolor::yellow;
            }else if(status == udho::logging::status::debug){
                _stream << termcolor::green;
            }else if(status == udho::logging::status::info){
                _stream << termcolor::blue;
            }
            _stream << "[" << status_str << "]";
            _stream << " ";
            _stream << message;
            _stream << termcolor::reset;
            _stream << std::endl;
        }
    }
    
    self_type& operator+=(const logging::features::colored& colored){
        _feature_color = colored;
        return *this;
    }
    
    template <udho::logging::status Status>
    self_type& operator()(const udho::logging::message<Status>& msg){
        log(msg.time, msg.status, msg._segment, msg._level, msg.what());
        return *this;
    }
};

typedef plain<std::ostream> ostream;
    
}
}

#endif // UDHO_LOGGING_H
