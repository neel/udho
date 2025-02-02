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

#ifndef UDHO_VIEW_BRIDGES_STREAM_H
#define UDHO_VIEW_BRIDGES_STREAM_H

#include <string>
#include <array>
#include <limits>
#include <vector>
#include <fstream>
#include <iomanip>
#include <stdexcept>
#include <exception>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string/trim_all.hpp>
#include <udho/url/detail/format.h>

namespace udho{
namespace view{
namespace data{
namespace bridges{

/**
 * @struct stream
 * @ingroup view
 * @brief A generic text stream class for handling formatted text output with controlled indentation.
 *
 * This template class facilitates structured text generation, which is particularly useful for scripting and code generation in various languages. It manages indentation and newlines to produce readable and well-formatted output.
 *
 * @tparam CharT Character type for the stream (default: char).
 * @tparam C Default indentation character (default: tab '\t').
 */
template <typename CharT = char, CharT C = '\t'>
struct stream{
    using char_type = CharT; ///< The character type used in the stream.
    static constexpr const char_type indent_char = C; ///< The character used for indentation.

    /**
     * @brief Constructs a new stream object with default settings.
     */
    explicit stream(): _indent(0), _empty_newline(true) {
        std::fill(_indent_str.begin(), _indent_str.end(), indent_char);
    }

    stream(const stream&) = delete; ///< Copy constructor is deleted.
    stream& operator=(const stream&) = delete; ///< Copy assignment is deleted.

    /**
     * @brief Appends a string to the stream.
     * @param s Reference to the stream.
     * @param str The string to append.
     * @return A reference to the modified stream.
     */
    friend stream& operator<<(stream& s, const std::string& str){
        s.append(str);
        return s;
    }

    /**
     * @brief Handles manipulators, specifically std::endl to append a newline.
     * @param s Reference to the stream.
     * @param manip Manipulator function.
     * @return A reference to the modified stream.
     */
    friend stream& operator<<(stream& s, std::ostream& (*manip)(std::ostream&)) {
        if (manip == static_cast<std::ostream& (*)(std::ostream&)>(std::endl)) {
            s.append_newline();
        }
        return s;
    }

    /**
     * @brief Prefix increment to increase indentation.
     */
    stream& operator++() {
        indent(true);
        return *this;
    }

    /**
     * @brief Postfix increment to increase indentation.
     */
    stream operator++(int) {
        stream temp = *this;
        indent(true);
        return temp;
    }

    /**
     * @brief Prefix decrement to decrease indentation.
     */
    stream& operator--() {
        indent(false);
        return *this;
    }

    /**
     * @brief Postfix decrement to decrease indentation.
     */
    stream operator--(int) {
        stream temp = *this;
        indent(false);
        return temp;
    }

    /**
     * @brief Returns the entire content of the buffer as a standard string.
     * @return A string containing all characters currently in the buffer.
     */
    std::string body() const { return std::string(_buffer.begin(), _buffer.end()); }
    /**
     * @brief Provides access to the raw data of the buffer.
     * @return A pointer to the beginning of the data buffer.
     */
    const char* data() const { return _buffer.data(); }
    /**
     * @brief Gets the current size of the buffer.
     * @return The size of the buffer in characters.
     */
    std::size_t size() const { return _buffer.size(); }

    std::string save(const std::string& name) const {
        std::string sanitized_name = name;
        std::replace_if(sanitized_name.begin(), sanitized_name.end(), [](char ch) {
            return !std::isalnum(ch) && ch != '.' && ch != '-' && ch != '_';
        }, '_');
        boost::filesystem::path temp_dir = boost::filesystem::temp_directory_path();
        boost::filesystem::path temp_file = temp_dir / boost::filesystem::unique_path(udho::url::format("{}-%%%%-%%%%-%%%%-%%%%.lua", sanitized_name));

        std::ofstream out(temp_file.string());
        if (!out) {
            throw std::runtime_error("Failed to create temporary file");
        }

        out.write(data(), size());
        if (!out.good()) {
            throw std::runtime_error("Failed to write data to temporary file");
        }
        out.close();

        return temp_file.string();
    }

    protected:
        /**
         * @brief Adjusts the indentation level of the output.
         * @param positive If true, increases the indentation level; if false, decreases it.
         * @throw std::underflow_error If decreasing the indentation would result in a negative indentation level.
         */
        void indent(bool positive){
            std::int8_t indent = _indent;
            indent += positive ? +1 : -1;
            if(indent < 0){
                throw std::underflow_error{"indentation < 0 is illegal"};
            }
            _indent = indent;
        }
    protected:
        /**
         * @brief Appends a string directly to the buffer.
         * @param str The string to append to the buffer.
         */
        void append(const std::string& str) { append(str.begin(), str.end()); }
        /**
         * @brief Appends a range of characters to the buffer, applying indentation as necessary.
         * @tparam Iterator Type of the iterator.
         * @param begin Iterator pointing to the beginning of the character range.
         * @param end Iterator pointing to the end of the character range.
         * @note Inserts indentation characters at the beginning of a new line if the line is currently empty.
         */
        template <typename Iterator>
        void append(Iterator begin, Iterator end) {
            if(_empty_newline){
                _buffer.insert(_buffer.end(), _indent_str.begin(), _indent_str.begin() + _indent);
                _empty_newline = false;
            }
            _buffer.insert(_buffer.end(), begin, end);
        }
        /**
         * @brief Appends a newline character to the buffer and resets the line state to empty.
         * @details This causes the next line of output to begin with the appropriate indentation.
         */
        void append_newline() {
            _buffer.push_back('\n');
            _empty_newline = true;
        }
    private:
        std::vector<char_type> _buffer;  ///< The buffer storing the stream's content.
        std::int8_t _indent;  ///< Current indentation level.
        bool _empty_newline;  ///< Flag indicating whether the current line is empty.
        std::array<char_type, std::numeric_limits<std::int8_t>::max()> _indent_str;  ///< Array used for indentation.
};


}
}
}
}

#endif // UDHO_VIEW_BRIDGES_STREAM_H
