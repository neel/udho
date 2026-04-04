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

#ifndef UDHO_HAZO_MAP_IO_H
#define UDHO_HAZO_MAP_IO_H

#include <string>
#include <chrono>
#include <ostream>
#include <udho/hazo/map/element.h>
#include <udho/hazo/map/map.h>
#include <udho/utils/date_time.h>

namespace udho{
namespace hazo{

namespace detail{

template <typename ValueT, typename Enable = void>
struct value_printer{
    value_printer(const ValueT& v): _value(v) {}

    std::ostream& operator()(std::ostream& stream) const {
        stream << _value;
        return stream;
    }
private:
    const ValueT& _value;
};

template <typename ValueT>
struct value_printer<ValueT, std::enable_if_t<std::is_enum_v<ValueT>>>{
    value_printer(const ValueT& v): _value(v) {}

    std::ostream& operator()(std::ostream& stream) const {
        using underlying_t = std::underlying_type_t<ValueT>;
        stream << static_cast<underlying_t>(_value);
        return stream;
    }
private:
    const ValueT& _value;
};

template <typename Clock, typename Duration>
struct value_printer<std::chrono::time_point<Clock, Duration>>{
    value_printer(const std::chrono::time_point<Clock, Duration>& v): _value(v) {}

    std::ostream& operator()(std::ostream& stream) const {
        stream << udho::utils::date_time::format_rfc7231(_value);
        return stream;
    }

private:
    const std::chrono::time_point<Clock, Duration>& _value;
};

}

template <typename DerivedT, typename ValueT, template<class, typename> class... Mixins>
std::ostream& operator<<(std::ostream& stream, const element<DerivedT, ValueT, Mixins...>& elem){
    stream << "< " << elem.key().c_str() << ": ";
    detail::value_printer printer(elem.value());
    printer(stream);
    stream << ">";
    return stream;
}

template <typename Policy, typename... X>
std::ostream& operator<<(std::ostream& stream, const basic_map<Policy, X...>& s){
    stream << "(";
    s.write(stream);
    stream << ")";
    return stream;
}
    
}
}

#endif // UDHO_HAZO_MAP_IO_H


