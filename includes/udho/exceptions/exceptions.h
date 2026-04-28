#ifndef UDHO_EXCEPTIONS_H
#define UDHO_EXCEPTIONS_H

#include <exception>
#include <stdexcept>
#include <type_traits>
#include <boost/beast/http/status.hpp>
#include <cpptrace/cpptrace.hpp>
#include <cpptrace/from_current.hpp>

namespace udho{
namespace http{

class error : public std::runtime_error {
    boost::beast::http::status _status;

public:
    explicit error(boost::beast::http::status st, std::string msg = {})
        : std::runtime_error(msg.empty() ? std::string(boost::beast::http::obsolete_reason(st)) : std::move(msg))
        , _status(st)
    {}

    boost::beast::http::status status() const noexcept { return _status; }
    boost::beast::http::status_class status_class() const noexcept { return boost::beast::http::to_status_class(_status); }
};

}

namespace exceptions{

struct captured {
    using trace_type = cpptrace::raw_trace;

    static constexpr const std::size_t skip = 1;

    static captured propagate() {
        std::exception_ptr eptr = std::current_exception();

        if (!eptr) {
            throw std::logic_error("exceptions::captured::propagate() called without an active exception");
        }

        auto trace = cpptrace::raw_trace_from_current_exception();
        if (trace.empty()) {
            trace = cpptrace::generate_raw_trace(skip);
        }

        return captured(std::move(eptr), std::move(trace));
    }

    template <typename E>
    static captured propagate(E&& e) {
        return captured(std::forward<E>(e));
    }

    bool empty() const noexcept { return !_exception; }

    const trace_type& trace() const noexcept { return _trace; }

    const std::exception_ptr& exception() const noexcept { return _exception; }

    [[noreturn]] void rethrow() const {
        if (!_exception) {
            throw std::logic_error("exceptions::captured has no exception");
        }
        std::rethrow_exception(_exception);
    }

    bool operator!() const { return empty(); }

    operator bool() const { return !empty(); }

    void reset() {
        _exception = nullptr;
        _trace = {};
    }

public:
    captured() {}

    explicit captured(std::exception_ptr&& ptr, trace_type trace = cpptrace::generate_raw_trace(skip)) noexcept: _exception(std::move(ptr)), _trace(std::move(trace)) {}

    template <typename E, typename D = std::decay_t<E>, typename = std::enable_if_t<!std::is_same_v<D, captured> && !std::is_same_v<D, std::exception_ptr>>>
    explicit captured(E&& exception) {
        try {
            throw std::forward<E>(exception);
        } catch (...) {
            _exception = std::current_exception();
            _trace     = cpptrace::generate_raw_trace(skip);
        }
    }

private:
    std::exception_ptr   _exception;
    trace_type _trace;
};

}

}

#endif // UDHO_EXCEPTIONS_H
