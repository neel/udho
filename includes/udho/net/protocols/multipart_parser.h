#ifndef UDHO_NET_MULTIPART_PARSER_H
#define UDHO_NET_MULTIPART_PARSER_H

#include <variant>
#include <string>
#include <boost/filesystem.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/buffers_iterator.hpp>
#include <udho/utils/string_view.h>
#include <udho/utils/format.h>
#include <boost/algorithm/string/find.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/beast/core/buffers_prefix.hpp>
#include <deque>
#include <fstream>
#include <udho/net/protocols/form_data.h>
#include <udho/net/protocols/request_parser_config.h>

namespace udho{
namespace net{
namespace protocols{

namespace detail{

/** @addtogroup DoxyG_net
 *  @{
 */

/**
 * @brief Incremental parser for `multipart/form-data` content.
 *
 * This class implements a state machine that parses multipart data incrementally as
 * bytes become available. It is designed to work with both plain (Content-Length)
 * and chunked transfer encodings, feeding data from a buffer and advancing the parse
 * state accordingly.
 *
 * The parser recognizes the following structure (RFC 7578):
 *   - First boundary: `--boundary\r\n`
 *   - Part headers: `Content-Disposition: form-data; name="field"` etc., terminated by `\r\n\r\n`
 *   - Part data (field value or file content)
 *   - Subsequent boundaries: `\r\n--boundary\r\n`
 *   - Final boundary: `\r\n--boundary--\r\n`
 *
 * When a part is fully parsed, the parser updates the associated `form_data` object, storing
 * either a string value (for fields) or a filesystem path (for uploaded files). Files are written
 * incrementally to temporary files as data arrives.
 *
 * The parser never blocks; it returns `boost::asio::error::would_block` when it needs more data to
 * make progress. The caller should then supply additional data and invoke the parser again.
 *
 * @tparam Buffer The buffer type that provides the `data()` and `consume()`
 *                interface (e.g., `boost::beast::flat_buffer`). The parser
 *                consumes bytes from this buffer as it parses.
 *
 * @plantumlfile h11_multipart.puml ["Multipart parsing state machine"]
 */
template <typename Buffer>
struct multipart_parser{
    using buffer_type           = Buffer;
    using field_value_type      = udho::net::protocols::detail::field_value_type;

    /**
     * @brief Current state of the multipart parser.
     */
    enum class lookahead{
        none,                   ///< Initial state, before any data.
        boundary_first,         ///< Expecting the very first boundary (`--boundary`).
        boundary_intermediate,  ///< Expecting a subsequent boundary (`\r\n--boundary`).
        boundary_follow,        ///< After a boundary, expecting either `--` (final) or `\r\n`.
        meta_terminal,          ///< Reading part headers, expecting `\r\n\r\n`.
        data_field,             ///< Reading a text field's value (until next boundary).
        data_file,              ///< Reading a file's content (until next boundary).
        end,                    ///< Final boundary seen, expecting trailing `\r\n`.
        error                   ///< Parsing error encountered (protocol violation).
    };

    /**
     * @brief Construct a parser that stores results in the given form container.
     * @param form Reference to a `form_data` object that will hold the parsed fields/files.
     * @param config Body parser configuration.
     */
    explicit multipart_parser(udho::net::protocols::detail::form_data& form, const udho::net::detail::body_parser_config& config): _form(form), _config(config) {}

    multipart_parser(multipart_parser&& other): _form(other._form), _config(other._config) {
        reset();
    }

    /**
     * @brief Get the current lookahead state.
     * @return Current state.
     */
    lookahead ahead() const { return _lookahead; }

    /**
     * @brief Check whether parsing has completed successfully.
     * @return `true` if the final boundary and its trailing CRLF have been consumed.
     */
    bool finished() const { return _finished; }

    /**
     * @brief Set the boundary string to be used for parsing.
     * @param boundary The boundary including the leading `"--"` (e.g., `"--boundary123"`).
     *
     * @note This must be called before any data is fed to the parser.
     */
    void operator()(const std::string& boundary) {
        _boundary = boundary;
    }

    /**
     * @brief Feed a buffer of data to the parser.
     * @param buffer The buffer containing (part of) the multipart body.
     * @return
     *   - `boost::system::error_code{}` on success (parsing may still be incomplete).
     *   - `boost::asio::error::would_block` if more data is needed to make progress.
     *   - A non‑zero error code (e.g., `protocol_error`, `io_error`) on fatal error.
     *
     * The parser consumes bytes from the buffer as it processes them. After a
     * successful return, the caller should inspect `finished()` to determine
     * whether parsing is complete. If `would_block` is returned, the caller
     * should read more data into the buffer and invoke the parser again.
     */
    boost::system::error_code operator()(buffer_type& buffer) {
        return process(buffer);
    }

    /**
     * @brief Return the total number of bytes consumed from all buffers so far.
     * @return Bytes consumed (sum of all `buffer.consume()` calls).
     */
    std::size_t bytes_consumed() const { return _bytes_consumed; }

    /**
     * @brief Reset the parser to its initial state (clearing any stored boundary).
     *
     * This also resets the associated `form_data` and closes any open temporary file.
     */
    void reset() {
        _boundary       = "";
        _lookahead      = lookahead::none;
        _start          = 0;
        _bytes_consumed = 0;
        _finished       = false;

        _current_file.reset();
        _form.reset();
    }
private:

    /**
     * @brief Main parsing loop. Repeatedly calls the appropriate state handler
     *        until the buffer is exhausted or an error occurs.
     * @param buffer The data buffer to parse.
     * @return
     *   - `{}` if at least one token was successfully processed.
     *   - `would_block` if no progress could be made (need more data).
     *   - A non‑zero error code on fatal error.
     * @post The buffer may be partially consumed.
     * @note This function is called by the public `operator()`.
     */
    boost::system::error_code process(buffer_type& buffer) {
        if (_finished) return {};
        if (buffer.size() == 0) return boost::asio::error::would_block;

        if (_lookahead == lookahead::none) _lookahead = lookahead::boundary_first;

        boost::system::error_code ec;

        while(true) {
            if(_finished) break;

            switch (_lookahead) {
                case lookahead::boundary_first:
                case lookahead::boundary_intermediate: ec = _header(buffer); break;
                case lookahead::boundary_follow:       ec = _follow(buffer); break;
                case lookahead::meta_terminal:         ec = _meta(buffer); break;
                case lookahead::data_field:            ec = _field(buffer); break;
                case lookahead::data_file:             ec = _file(buffer); break;
                case lookahead::end:                   ec = _end(buffer); break;
                case lookahead::error:                 return boost::system::errc::make_error_code(boost::system::errc::protocol_error);
                default:                               return boost::system::errc::make_error_code(boost::system::errc::protocol_error);
            }

            if(ec) break;
        }
        return ec;
    }

    /**
     * @brief Look for the expected boundary in the buffer.
     * @param buffer The data buffer.
     * @return
     *   - `{}` if the boundary was found and consumed.
     *   - `would_block` if more data is needed (boundary not found or incomplete).
     * @pre State is either `boundary_first` or `boundary_intermediate`.
     * @post If successful, the boundary is consumed and state advances to `boundary_follow`.
     * @post If `would_block`, the buffer is not consumed and `_start` is adjusted to
     *       retain a possible partial boundary at the end.
     */
    boost::system::error_code _header(buffer_type& buffer) {
        assert(_lookahead == lookahead::boundary_first || _lookahead == lookahead::boundary_intermediate);

        std::string expected_delim = ((_lookahead == lookahead::boundary_first) ? _boundary : "\r\n"+_boundary);

        const auto& cbuff = buffer.data();
        auto begin = boost::asio::buffers_begin(cbuff);
        std::advance(begin, _start);
        auto end   = boost::asio::buffers_end(cbuff);

        auto cbuff_it = std::search(begin, end, expected_delim.cbegin(), expected_delim.cend());

        std::size_t distance = std::distance(begin, cbuff_it);

        if(cbuff_it == end) {
            // boundary not found
            // need to read more
            // don't consume the buffer
            std::size_t safe_residue = std::min(expected_delim.size() -1, distance);
            _start = _start + distance - safe_residue;

            return boost::asio::error::would_block;
        } else {
            // boundary found
            // consume till boundary

            std::size_t consumable_bytes = _start + distance + expected_delim.size();
            buffer.consume(consumable_bytes);
            _bytes_consumed += consumable_bytes;

            _start = 0; // reset

            _lookahead = lookahead::boundary_follow;

            return {}; // process(buffer)
        }
    }

    /**
     * @brief Examine the two bytes after a boundary to determine what comes next.
     * @param buffer The data buffer.
     * @return
     *   - `{}` on success (two bytes consumed, state updated).
     *   - `would_block` if less than two bytes are available.
     *   - `protocol_error` if the two bytes are neither `--` nor `\r\n`.
     * @pre State is `boundary_follow`.
     * @post If the two bytes are `--`, state advances to `end`.
     * @post If the two bytes are `\r\n`, state advances to `meta_terminal`.
     * @post Exactly two bytes are consumed from the buffer.
     */
    boost::system::error_code _follow(buffer_type& buffer) {
        assert(_lookahead == lookahead::boundary_follow);

        if(buffer.size() < 2) return boost::asio::error::would_block;

        const auto& cbuff = buffer.data();
        auto begin = boost::asio::buffers_begin(cbuff);
        auto end   = boost::asio::buffers_end(cbuff);

        std::size_t remaining_bytes = std::distance(begin, end);

        if(remaining_bytes < 2) {
            return boost::asio::error::would_block;
        }

        auto terminal_it = begin;
        std::advance(terminal_it, 2);

        std::string two_bytes(begin, terminal_it);

        buffer.consume(2);
        _bytes_consumed += 2;

        if(two_bytes == "--") {
            _lookahead = lookahead::end;
            return {}; // process(buffer);
        } else if(two_bytes == "\r\n") {
            _lookahead = lookahead::meta_terminal;
            return {}; // process(buffer);
        } else {
            _lookahead = lookahead::error;

            return boost::system::errc::make_error_code(boost::system::errc::protocol_error);
        }
    }

    /**
     * @brief Read the trailing CRLF after the final boundary (`--boundary--`).
     * @param buffer The data buffer.
     * @return
     *   - `{}` on success (final CRLF consumed, parsing finished).
     *   - `would_block` if less than two bytes are available.
     *   - `protocol_error` if the two bytes are not `\r\n`.
     * @pre State is `end`.
     * @post On success, `_finished = true` and the buffer is consumed by two bytes.
     */
    boost::system::error_code _end(buffer_type& buffer) {
        assert(_lookahead == lookahead::end);

        if(_finished) return {};

        const auto& cbuff = buffer.data();
        auto begin = boost::asio::buffers_begin(cbuff);
        auto end   = boost::asio::buffers_end(cbuff);

        std::size_t remaining_bytes = std::distance(begin, end);

        if(remaining_bytes < 2) {
            return boost::asio::error::would_block;
        }

        udho::utils::string_view expected_str = "\r\n";

        auto terminal_it = begin;
        std::advance(terminal_it, 2);

        std::string buffer_data(begin, terminal_it);

        if(expected_str != buffer_data) {
            _lookahead = lookahead::error;
            return boost::system::errc::make_error_code(boost::system::errc::protocol_error);
        }

        buffer.consume(2);
        _bytes_consumed += 2;

        _finished = true;

        return {};
    }

    /**
     * @brief Search for the end of part headers (`\r\n\r\n`).
     * @param buffer The data buffer.
     * @return
     *   - `{}` if the delimiter was found and headers extracted.
     *   - `would_block` if more data is needed.
     *   - `protocol_error` if headers are malformed (delegated to `_meta_extract`).
     * @pre State is `meta_terminal`.
     * @post If successful, the delimiter and preceding headers are consumed,
     *       and state is set by `_meta_extract` (to either `data_field` or `data_file`).
     * @post If `would_block`, the buffer is not consumed and `_start` retains
     *       a possible partial delimiter.
     */
    boost::system::error_code _meta(buffer_type& buffer) {
        assert(_lookahead == lookahead::meta_terminal);

        std::string expected_delim = "\r\n\r\n";

        const auto& cbuff = buffer.data();
        auto begin = boost::asio::buffers_begin(cbuff);
        std::advance(begin, _start);
        auto end   = boost::asio::buffers_end(cbuff);

        auto cbuff_it = std::search(begin, end, expected_delim.cbegin(), expected_delim.cend());
        std::size_t distance = std::distance(begin, cbuff_it);

        if(cbuff_it == end) {
            // delim not found
            // need to read more
            // don't consume the buffer
            std::size_t safe_residue = std::min(expected_delim.size() -1, distance);
            _start = _start + distance - safe_residue;

            return boost::asio::error::would_block;
        } else {
            // delim found
            // consume till boundary

            std::size_t consumable_bytes = _start + distance + expected_delim.size();

            std::string meta(distance, '\0');
            meta.reserve(distance);
            meta.assign(boost::asio::buffers_begin(cbuff), cbuff_it);

            auto error = _meta_extract(meta);
            buffer.consume(consumable_bytes);
            _bytes_consumed += consumable_bytes;

            _start = 0; // reset

            if(!error) {
                return {}; // process(buffer);
            } else {
                return error;
            }
        }
    }

    /**
     * @brief Parse the collected headers to extract field name and optional filename.
     * @param meta A string view containing the headers (up to `\r\n\r\n`).
     * @return
     *   - `{}` on success.
     *   - `protocol_error` if required fields are missing or malformed.
     * @pre `meta` contains at least one header line.
     * @post If a filename is present, a temporary file is created and the stream
     *       is stored in `_current_file`; state becomes `data_file`.
     * @post If no filename, a new string field is emplaced in `_form`; state becomes `data_field`.
     */
    boost::system::error_code _meta_extract(udho::utils::string_view meta) {
        auto range = boost::ifind_first(meta, "Content-Disposition:");
        if(range.empty()) {
            _lookahead = lookahead::error;
            return boost::system::errc::make_error_code(boost::system::errc::protocol_error);
        }

        std::size_t disposition_pos = std::distance(meta.begin(), range.end());
        std::size_t crlf_pos        = meta.find("\r\n", disposition_pos);       // must exist because we are using async_read_until with "\r\n\r\n"
        udho::utils::string_view disposition     = meta.substr(disposition_pos, (crlf_pos - disposition_pos));

        std::deque<std::string> disposition_parts;
        boost::split(disposition_parts, disposition, boost::is_any_of(";"));
        std::string form_data = disposition_parts.front();
        boost::trim(form_data);
        if(!boost::iequals(form_data, "form-data")) {
            _lookahead = lookahead::error;
            return boost::system::errc::make_error_code(boost::system::errc::protocol_error);
        }
        disposition_parts.pop_front();

        std::string name;
        std::string filename;

        for (std::string& disposition_part : disposition_parts) {
            auto pos = disposition_part.find('=');
            if (pos == std::string::npos)
                continue;
            std::string key   = disposition_part.substr(0,pos);
            std::string value = disposition_part.substr(pos+1);

            boost::trim(key);
            boost::trim(value);
            boost::trim_if(value, boost::is_any_of("\""));

            if (boost::iequals(key, "name")) name = value;
            if (boost::iequals(key, "filename")) filename = value;
        }

        if(name.empty()) {
            _lookahead = lookahead::error;
            return boost::system::errc::make_error_code(boost::system::errc::protocol_error);
        }

        if(filename.empty()) {
            std::string value;
            field_value_type field_value(name, std::move(value));
            _form.emplace(name, std::move(field_value));

            _lookahead = lookahead::data_field;
        } else {
            if(name.empty()) {
                _lookahead = lookahead::error;
                return boost::system::errc::make_error_code(boost::system::errc::protocol_error);
            }

            if(_config.upload_in_buffer()) {
                field_value_type field_value(name, boost::beast::multi_buffer{});
                _form.emplace(name, std::move(field_value));
            } else {
                auto safe_filename      = udho::utils::filesystem::path(filename).filename().string();
                auto unique_file_name   = udho::utils::format("%%%%-%%%%-%%%%-%%%%-{}", safe_filename);
                auto temp_dir           = udho::utils::filesystem::temp_directory_path();
                auto temp_file          = temp_dir / boost::filesystem::unique_path(unique_file_name).string();

                std::unique_ptr<std::ofstream> ostream  = std::make_unique<std::ofstream>(temp_file.string().c_str(), std::ios::binary);

                field_value_type field_value(name, std::move(temp_file));
                _form.emplace(name, std::move(field_value));
                _current_file = std::move(ostream);
            }
            _lookahead = lookahead::data_file;
        }
        return {};
    }

    /**
     * @brief Read data for a text field until the next boundary.
     * @param buffer The data buffer.
     * @return
     *   - `{}` on success (some data appended, possibly boundary found).
     *   - `would_block` if no data could be appended (boundary not found and buffer empty).
     *   - `value_too_large` if the value crosses the `field_content_limit` threshold
     * @pre State is `data_field` and `_form.current()` points to a string variant.
     * @post Data up to (but not including) the next boundary is appended to the current field.
     * @post If a boundary is found and verified, state advances to `boundary_intermediate`.
     * @post Otherwise, state remains `data_field` and the buffer may be partially consumed.
     */
    boost::system::error_code _field(buffer_type& buffer) {
        assert(_lookahead == lookahead::data_field);
        assert(_form.current()->second.has_string());

        const auto& cbuff = buffer.data();
        auto begin = boost::asio::buffers_begin(cbuff);
        auto end   = boost::asio::buffers_end(cbuff);

        auto [boundary_found, it] = _attachment(begin, end);

        std::size_t length = std::distance(begin, it);

        if(length > 0) {
            auto& value = _form.current()->second;
            std::string& str = value.string();

            if(_config.field_content_limit() > 0 && (str.size() + length) > _config.field_content_limit()) {
                return boost::system::errc::make_error_code(boost::system::errc::value_too_large);
            }

            str.append(begin, it);
            buffer.consume(length);
            _bytes_consumed += length;
        }

        if(boundary_found) {
            // case 1F
            // we have already consumed length
            // but length is distance(begin, it)
            // and it is the begining of delim_str
            // But delim_str should be consumed by read_plain_multipart_header
            // so here we don't consume the delim_str.size()
            _lookahead = lookahead::boundary_intermediate;
        } else {
            if(length == 0) return boost::asio::error::would_block;
        }

        return {}; // process(buffer);
    }

    /**
     * @brief Scan the buffer for the next boundary, verifying the two‑byte suffix.
     * @param begin Iterator to start of buffer.
     * @param end   Iterator to end of buffer.
     * @return A pair `(boundary_found, iterator)` where:
     *         - `boundary_found` is true if a verified boundary was located.
     *         - `iterator` points either to the start of the boundary (if found)
     *           or to a position where scanning should resume (adjusted for partial matches).
     * @note This helper is used by both `_field` and `_file`.
     */
    template <typename Iterator>
    auto _attachment(Iterator begin, Iterator end) {
        std::string delim_str = "\r\n"+_boundary;

        bool boundary_not_found = false;
        auto it = begin;
        while(true) {
            it = std::search(it, end, delim_str.begin(), delim_str.end());
            // possible values of it
            //  a) end                                                      # if not matched
            //  b) it <= x < end where distance(x, end) >= delim_str.size() # if matched

            // in case of (b) delim_str.size() > 2
            // therefore distance(x, end) >= 2
            // in case of (a) distance(x, end) == 0

            // check next two characters after delim_str
            // if it is \r\n or -- then real boundary is
            // found, otherwise it is not

            if(it == end) {                                 // case (a)
                boundary_not_found = true;                  // assignment 1T
                break;
            } else {                                        // case (b)
                auto delim_end = it + delim_str.size();     // legal because of (b)
                std::size_t leftover_size = std::distance(delim_end, end);
                if(leftover_size < 2) {
                    boundary_not_found = true;              // assignment 2T
                    // break because delim_str.size() > 2
                    // repeating the loop wont take us anywhere
                    break;
                } else {
                    auto terminal_it = delim_end;
                    std::advance(terminal_it, 2);
                    std::string two_chars(delim_end, terminal_it);
                    if(two_chars == "\r\n" || two_chars == "--") {
                        boundary_not_found = false;         // assignment 1F
                        break;
                    } else {
                        // unintended boundary
                        boundary_not_found = true;          // assignment 3T
                    }
                    std::advance(it, 1);
                }
            }
        }

        // We did not check for partial matches with the boundary
        // We leave that for the next run of async_read_some
        // Therefore we leave (delim_str.size()-1) bytes in the buffer
        // However, if distance(begin, it) < (delim_str.size()-1)
        // then we leave the whole for the next run if boundary_not_found
        if(boundary_not_found) {
            std::size_t safe_partial_leftover_size = std::min<std::size_t>((delim_str.size()-1), std::distance(begin, it));
            it = it - safe_partial_leftover_size;
        }

        bool boundary_found = !boundary_not_found;

        return std::make_pair(boundary_found, it);
    }

    /**
     * @brief Read data for a file part until the next boundary.
     * @param buffer The data buffer.
     * @return
     *   - `{}` on success (data written to file, possibly boundary found).
     *   - `would_block` if no data could be written (boundary not found and buffer empty).
     *   - `io_error` if a write to the temporary file fails.
     *   - `file_too_large` is file contents exceeds `field_content_limit` configuration parameter and field_content_limit > 0
     * @pre State is `data_file` and `_form.current()` points to a path variant.
     * @post Data up to (but not including) the next boundary is written to `_current_file`.
     * @post If a boundary is found and verified, the file is flushed, closed, and
     *       state advances to `boundary_intermediate`.
     * @post Otherwise, state remains `data_file` and the buffer may be partially consumed.
     */
    boost::system::error_code _file(buffer_type& buffer) {
        assert(_lookahead == lookahead::data_file);
        if (_config.upload_in_buffer())
            assert(_form.current()->second.is_buffer());
        else
            assert(_form.current()->second.is_path());

        const auto& cbuff = buffer.data();
        auto buff_begin = boost::asio::buffers_begin(cbuff);
        auto buff_end   = boost::asio::buffers_end(cbuff);

        auto [boundary_found, buff_it] = _attachment(buff_begin, buff_end);

        std::size_t length = std::distance(buff_begin, buff_it);

        if(length > 0) {
            auto seq = boost::beast::buffers_prefix(length, cbuff);
            for (auto it = boost::asio::buffer_sequence_begin(seq); it != boost::asio::buffer_sequence_end(seq); ++it) {
                const auto& segment = *it;
                auto p = static_cast<char const*>(segment.data());
                auto n = static_cast<std::size_t>(segment.size());

                if(_config.upload_in_buffer()) {
                    auto& value = _form.current()->second;
                    boost::beast::multi_buffer& buffer = value.buffer();

                    if(_config.field_content_limit() > 0 && (buffer.size() + n) > _config.field_content_limit()) {
                        return boost::system::errc::make_error_code(boost::system::errc::file_too_large);
                    }

                    auto mutable_buffer = buffer.prepare(n);
                    boost::asio::buffer_copy(mutable_buffer, boost::asio::buffer(p, n));
                    buffer.commit(n);
                } else {
                    std::streampos start_position = 0; // guranteed to be a new file (no existing file gets appended during teh upload)
                    std::streampos write_position = _current_file->tellp();
                    if(write_position < 0) {
                        return boost::system::errc::make_error_code(boost::system::errc::io_error);
                    }

                    std::size_t    write_buffer_length = write_position - start_position;

                    if(_config.field_content_limit() > 0 && (write_buffer_length + n) > _config.field_content_limit()) {
                        return boost::system::errc::make_error_code(boost::system::errc::file_too_large);
                    }

                    _current_file->write(p, n);

                    if (!_current_file->good())
                        return boost::system::errc::make_error_code(boost::system::errc::io_error);
                }
            }

            buffer.consume(length);
            _bytes_consumed += length;
        }

        if(boundary_found) {
            // case 1F
            // we have already consumed length
            // but length is distance(begin, it)
            // and it is the begining of delim_str
            // But delim_str should be consumed by read_plain_multipart_header
            // so here we don't consume the delim_str.size()

            if(_config.upload_in_buffer()) {
                // auto& value = _form.current()->second;
                // boost::beast::multi_buffer& buffer = value.buffer();
                // buffer.shrink_to_fit();
            } else {
                _current_file->flush();
                if (!_current_file->good()) {
                    return boost::system::errc::make_error_code(boost::system::errc::io_error);
                }
                _current_file.reset();
            }

            _lookahead = lookahead::boundary_intermediate;
        } else {
            if(length == 0) return boost::asio::error::would_block;
        }

        return {}; // process(buffer);
    }

private:
    std::string _boundary;
    lookahead   _lookahead      = lookahead::none;
    std::size_t _bytes_consumed = 0;
    std::size_t _start          = 0;
    bool        _finished       = false;
    form_data&  _form;
    const udho::net::detail::body_parser_config& _config;
    std::unique_ptr<std::ofstream>  _current_file;
};

/** @} */

}

}
}
}

#endif // UDHO_NET_MULTIPART_PARSER_H
