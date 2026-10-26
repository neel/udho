// SPDX-FileCopyrightText: 2026 Neel Basu <email>
// SPDX-License-Identifier: BSD-3-Clause

#ifndef UDHO_VIEW_BRIDGES_EXCEPTIONS_H
#define UDHO_VIEW_BRIDGES_EXCEPTIONS_H

#include <cstddef>
#include <exception>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <udho/view/bridges/source_map.h>

namespace udho{
namespace view{
namespace data{
namespace bridges{

/**
 * @brief Context captured while rendering one view.
 */
struct rendering_frame{
    struct source_line{
        std::size_t number;
        std::string content;
    };

    using source_lines_type = std::vector<source_line>;

    explicit rendering_frame(std::string view, std::string partial_output = {}, std::size_t generated_line = 0, source_span view_lines = {}, source_lines_type source_lines = {}, std::string traceback = {})
        : _view(std::move(view)), _partial_output(std::move(partial_output)), _generated_line(generated_line), _view_lines(view_lines), _source_lines(std::move(source_lines)), _traceback(std::move(traceback))
    {}

    inline const std::string& view() const noexcept { return _view; }
    inline const std::string& partial_output() const noexcept { return _partial_output; }
    inline std::size_t generated_line() const noexcept { return _generated_line; }
    inline const source_span& view_lines() const noexcept { return _view_lines; }
    inline const source_lines_type& source_lines() const noexcept { return _source_lines; }
    inline const std::string& traceback() const noexcept { return _traceback; }

private:
    std::string _view;
    std::string _partial_output;
    std::size_t _generated_line;
    source_span _view_lines;
    source_lines_type _source_lines;
    std::string _traceback;
};

/**
 * @brief A rendering failure together with the views through which it propagated.
 */
class rendering_error: public std::runtime_error{
public:
    using trace_type = std::vector<rendering_frame>;

    /**
     * @param root_cause The originating non-rendering C++ exception, or null for a Lua error.
     * @pre root_cause must not contain another rendering_error.
     */
    rendering_error(rendering_frame frame, std::string diagnostic, std::exception_ptr root_cause = nullptr)
        : std::runtime_error(format_message(frame, diagnostic)), _diagnostic(std::move(diagnostic)), _frame(std::move(frame)), _root_cause(std::move(root_cause))
    {}

    rendering_error(rendering_frame frame, const rendering_error& nested)
        : std::runtime_error(format_message(frame, nested._diagnostic)), _diagnostic(nested._diagnostic), _frame(std::move(frame)), _root_cause(nested._root_cause) {
        _trace.reserve(nested._trace.size() + 1);
        _trace.emplace_back(nested._frame);
        _trace.insert(_trace.end(), nested._trace.begin(), nested._trace.end());
    }

    inline const rendering_frame& frame() const noexcept { return _frame; }
    /**
     * @brief Nested view frames ordered from the immediate child to the originating view.
     */
    inline const trace_type& trace() const noexcept { return _trace; }
    inline const std::string& diagnostic() const noexcept { return _diagnostic; }
    inline const std::exception_ptr& root_cause() const noexcept { return _root_cause; }

private:
    static std::string format_message(const rendering_frame& frame, const std::string& diagnostic){
        std::string message = "Error rendering view '" + frame.view() + "'";
        if(frame.view_lines().valid()){
            if(frame.view_lines().begin == frame.view_lines().end){
                message += " at view line " + std::to_string(frame.view_lines().begin);
            } else {
                message += " near view lines " + std::to_string(frame.view_lines().begin) + "-" + std::to_string(frame.view_lines().end);
            }
        }

        if(!diagnostic.empty()){
            message += ": " + diagnostic;
        }
        return message;
    }

private:
    std::string _diagnostic;
    rendering_frame _frame;
    trace_type _trace;
    std::exception_ptr _root_cause;
};

}
}
}
}

#endif // UDHO_VIEW_BRIDGES_EXCEPTIONS_H
