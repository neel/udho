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

#ifndef UDHO_VIEW_BRIDGES_SCRIPT_H
#define UDHO_VIEW_BRIDGES_SCRIPT_H

#include <string>
#include <array>
#include <limits>
#include <vector>
#include <iomanip>
#include <stdexcept>
#include <udho/view/data/metatype.h>
#include <udho/view/sections.h>

namespace udho{
namespace view{
namespace data{
namespace bridges{

template <typename CharT = char, CharT C = '\t'>
struct stream{
    using char_type = CharT;
    static constexpr const char_type indent_char = C;

    explicit stream(): _indent(0), _empty_newline(true) {
        std::fill(_indent_str.begin(), _indent_str.end(), indent_char);
    }

    stream(const stream&) = delete;
    stream& operator=(const stream&) = delete;

    friend stream& operator<<(stream& s, const std::string& str){
        s.append(str);
        return s;
    }
    friend stream& operator<<(stream& s, std::ostream& (*manip)(std::ostream&)) {
        if (manip == static_cast<std::ostream& (*)(std::ostream&)>(std::endl)) {
            s.append_newline();
        }
        return s;
    }

    stream& operator++() {
        indent(true);
        return *this;
    }

    stream operator++(int) {
        stream temp = *this;
        indent(true);
        return temp;
    }

    stream& operator--() {
        indent(false);
        return *this;
    }

    stream operator--(int) {
        stream temp = *this;
        indent(false);
        return temp;
    }

    std::string body() const { return std::string(_buffer.begin(), _buffer.end()); }
    const char* data() const { return _buffer.data(); }
    std::size_t size() const { return _buffer.size(); }

    protected:
        void indent(bool positive){
            std::int8_t indent = _indent;
            indent += positive ? +1 : -1;
            if(indent < 0){
                throw std::underflow_error{"indentation < 0 is illegal"};
            }
            _indent = indent;
        }
    protected:
        void append(const std::string& str) { append(str.begin(), str.end()); }
        template <typename Iterator>
        void append(Iterator begin, Iterator end) {
            if(_empty_newline){
                _buffer.insert(_buffer.end(), _indent_str.begin(), _indent_str.begin() + _indent);
                _empty_newline = false;
            }
            _buffer.insert(_buffer.end(), begin, end);
        }
        void append_newline() {
            _buffer.push_back('\n');
            _empty_newline = true;
        }
    private:
        std::vector<char_type> _buffer;
        std::int8_t _indent;
        bool _empty_newline;
        std::array<char_type, std::numeric_limits<std::int8_t>::max()> _indent_str;
};

struct script: stream<char, '\t'>{
    explicit script(const std::string& name): stream(), _name(name) {}
    std::string name() const { return _name; }
    protected:
        inline void accept(const udho::view::sections::section& section){ stream::append(section.begin(), section.end()); }
        inline void discard(const udho::view::sections::section&){}
    private:
        std::string _name;
};

}
}
}
}

#endif // UDHO_VIEW_BRIDGES_SCRIPT_H

